#include "net/protocol.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════════
// Input tokenizer
//   Splits a line into tokens, supporting:
//     - whitespace delimiters (space, tab)
//     - single-quoted strings  'hello world'  → one token, no escape
//     - double-quoted strings  "hello \"world\""  → one token, \-escapes
// ═══════════════════════════════════════════════════════════════════════════════

static std::vector<std::string> tokenize(const std::string &line) {
  std::vector<std::string> tokens;
  size_t i = 0;

  while (i < line.size()) {
    // skip whitespace
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
      ++i;
    if (i >= line.size())
      break;

    std::string token;

    while (i < line.size() && line[i] != ' ' && line[i] != '\t') {
      if (line[i] == '"') {
        // double-quoted: support backslash escapes
        ++i;
        while (i < line.size() && line[i] != '"') {
          if (line[i] == '\\' && i + 1 < line.size()) {
            ++i;
            switch (line[i]) {
            case 'n':
              token += '\n';
              break;
            case 't':
              token += '\t';
              break;
            case 'r':
              token += '\r';
              break;
            default:
              token += line[i];
              break;
            }
          } else {
            token += line[i];
          }
          ++i;
        }
        if (i < line.size())
          ++i; // skip closing '"'
      } else if (line[i] == '\'') {
        // single-quoted: literal, no escapes
        ++i;
        while (i < line.size() && line[i] != '\'') {
          token += line[i++];
        }
        if (i < line.size())
          ++i; // skip closing '\''
      } else {
        token += line[i++];
      }
    }

    if (!token.empty())
      tokens.push_back(std::move(token));
  }

  return tokens;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Build a RESP Array-of-BulkStrings from a token list
// ═══════════════════════════════════════════════════════════════════════════════

static std::vector<uint8_t>
encode_command(const std::vector<std::string> &tokens) {
  std::vector<RespValue> elems;
  elems.reserve(tokens.size());
  for (const auto &t : tokens) {
    elems.push_back(RespValue::make_bulk_string(t));
  }
  std::vector<uint8_t> out;
  RespWriter::write(out, RespValue::make_array(std::move(elems)));
  return out;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Pretty-print a parsed RESP response
// ═══════════════════════════════════════════════════════════════════════════════

static void print_response(const RespValue &v, int indent = 0) {
  std::string pad(indent * 2, ' ');

  switch (v.type) {
  case RespType::SimpleString:
    std::cout << pad << "\033[32m" << v.str << "\033[0m\n";
    break;

  case RespType::Error:
    std::cout << pad << "\033[31m(error) " << v.str << "\033[0m\n";
    break;

  case RespType::Integer:
    std::cout << pad << "\033[33m(integer) " << v.integer << "\033[0m\n";
    break;

  case RespType::BulkString:
    if (v.is_null)
      std::cout << pad << "\033[90m(nil)\033[0m\n";
    else
      std::cout << pad << "\"" << v.str << "\"\n";
    break;

  case RespType::Array:
    if (v.is_null) {
      std::cout << pad << "\033[90m(empty array)\033[0m\n";
    } else if (v.array.empty()) {
      std::cout << pad << "(empty list or set)\n";
    } else {
      for (size_t i = 0; i < v.array.size(); ++i) {
        std::cout << pad << (i + 1) << ") ";
        print_response(v.array[i], indent + 1);
      }
    }
    break;

  case RespType::Null:
    std::cout << pad << "\033[90m(null)\033[0m\n";
    break;

  case RespType::Double:
    std::cout << pad << "\033[33m(double) " << v.dbl << "\033[0m\n";
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Blocking recv until we have a complete RESP message
//   We keep appending into `recv_buf` and attempt RespParser::parse.
//   On RespNotEnoughException we read more. On success we return the value.
// ═══════════════════════════════════════════════════════════════════════════════

static bool recv_response(int fd, std::vector<uint8_t> &recv_buf) {
  while (true) {
    // Try to parse whatever is already buffered
    if (!recv_buf.empty()) {
      size_t ptr = 0;
      try {
        RespValue resp = RespParser::parse(recv_buf, ptr);
        // Consume parsed bytes
        recv_buf.erase(recv_buf.begin(),
                       recv_buf.begin() + static_cast<std::ptrdiff_t>(ptr));
        std::cout << "\n";
        print_response(resp);
        return true;
      } catch (const RespNotEnoughException &) {
        // Need more data – fall through to recv()
      } catch (const RespParseException &e) {
        std::cerr << "\033[31m[parse error] " << e.what() << "\033[0m\n";
        recv_buf.clear();
        return false;
      }
    }

    // Read more bytes from the socket
    uint8_t tmp[64 * 1024];
    ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
    if (n > 0) {
      recv_buf.insert(recv_buf.end(), tmp, tmp + n);
    } else if (n == 0) {
      std::cout << "\033[33mServer closed the connection.\033[0m\n";
      return false;
    } else {
      if (errno == EINTR)
        continue;
      perror("recv");
      return false;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════════════════

int main(int argc, char *argv[]) {
  const char *host = "127.0.0.1";
  int port = 8080;

  if (argc >= 2)
    host = argv[1];
  if (argc >= 3)
    port = std::stoi(argv[2]);

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return 1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (::inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
    perror("inet_pton");
    ::close(fd);
    return 1;
  }

  if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
    perror("connect");
    ::close(fd);
    return 1;
  }

  std::cout << "Connected to " << host << ":" << port << "\n";
  std::cout << "Type commands (e.g.  SET key value) or 'exit' to quit.\n\n";

  std::vector<uint8_t> recv_buf; // persistent across iterations
  std::string line;

  while (true) {
    std::cout << host << ":" << port << "> ";
    std::cout.flush();

    if (!std::getline(std::cin, line))
      break; // EOF / Ctrl-D

    // trim trailing CR (Windows line endings)
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line == "exit" || line == "quit") {
      std::cout << "Bye!\n";
      break;
    }
    if (line.empty())
      continue;

    // ── Tokenize & encode ──────────────────────────────────────────────────
    auto tokens = tokenize(line);
    if (tokens.empty())
      continue;

    auto payload = encode_command(tokens);
    // ── Send ───────────────────────────────────────────────────────────────
    size_t sent_total = 0;
    while (sent_total < payload.size()) {
      ssize_t n = ::send(fd, payload.data() + sent_total,
                         payload.size() - sent_total, MSG_NOSIGNAL);
      if (n <= 0) {
        if (n == -1 && errno == EINTR)
          continue;
        perror("send");
        goto done;
      }
      sent_total += static_cast<size_t>(n);
    }

    // ── Receive & print response ───────────────────────────────────────────
    if (!recv_response(fd, recv_buf))
      break;
  }

done:
  ::close(fd);
  return 0;
}