#include "net/tcp_server.hpp"
#include "command/command.hpp"
#include "command/command_executer.hpp"
#include "command/command_parser.hpp"
#include "io/epoll.hpp"
#include "io/poll_event.hpp"
#include "lib/utils.hpp"
#include "logger/logger.hpp"
#include "net/protocol.hpp"
#include "net/tcp_connection.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <system_error>

TcpServer::TcpServer(const char *addr, int port) : poller(MAX_POLL_EVENTS) {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    throw std::system_error(errno, std::generic_category(), "socket() failed");
  }

  // RAII guard during server setup to avoid descriptor leak on error
  Socket server_socket(sock_fd);

  int opt = 1;
  if (setsockopt(server_socket.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "setsockopt(SO_REUSEADDR) failed");
  }

  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(port);

  int pton_res = inet_pton(AF_INET, addr, &listen_addr.sin_addr);
  if (pton_res == 0) {
    throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                            "invalid IP address: " + std::string(addr));
  } else if (pton_res < 0) {
    throw std::system_error(errno, std::generic_category(), "inet_pton failed");
  }

  server_socket.set_nonblocking();

  if (bind(server_socket.get_fd(), (sockaddr *)&listen_addr,
           sizeof(listen_addr)) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "bind server failed");
  }

  if (listen(server_socket.get_fd(), SOCK_MAX_CONN) == -1) {
    throw std::system_error(errno, std::generic_category(), "listen failed");
  }

  poller.add_poll_interest(
      PollInterest(server_socket.get_fd(), POLL_WANT_READ, PollTrigger::Edge));

  listen_fd = server_socket.release();
}

TcpServer::TcpServer() : TcpServer("0.0.0.0", 8080) {}

void TcpServer::start() {
  std::vector<PollEvent> events(MAX_POLL_EVENTS);
  while (true) {
    int event_count = poller.poll(events);
    for (int i = 0; i < event_count; i++) {
      int fd = events[i].fd;

      if (fd == listen_fd) {
        accept_connection();
        continue;
      }

      auto it = connections.find(fd);
      if (it == connections.end()) {
        continue;
      }
      TcpConnection *conn = it->second;

      if (events[i].mask & (POLL_ERROR | POLL_HANGUP)) {
        close_connection(fd);
        continue;
      }

      if (events[i].mask & POLL_READABLE) {
        TcpIOResult res = conn->read_buffer();
        if (res == TcpIOResult::CLOSED || res == TcpIOResult::ERROR) {
          close_connection(fd);
          continue;
        }

        if (res == TcpIOResult::DONE) {
          process_requests(conn);
        }

        if (conn->want_close()) {
          close_connection(fd);
          continue;
        }

        if (conn->want_write()) {
          uint32_t mask = POLL_WANT_READ | POLL_WANT_WRITE;
          poller.modify_poll_interest(
              PollInterest(fd, mask, PollTrigger::Edge));
        }
      }

      if (events[i].mask & POLL_WRITEABLE) {
        if (conn->want_write()) {
          TcpIOResult res = conn->write_buffer();
          if (res == TcpIOResult::CLOSED || res == TcpIOResult::ERROR) {
            close_connection(fd);
            continue;
          }
        }

        // If write drained fully, drop back to read-only interest
        if (!conn->want_write()) {
          poller.modify_poll_interest(
              PollInterest(fd, POLL_WANT_READ, PollTrigger::Edge));
        }
      }

      if (events[i].mask & POLL_SHUTDOWN) {
        close_connection(fd);
        continue;
      }
    }
  }
}

void TcpServer::process_requests(TcpConnection *conn) {
  auto &in_buf = conn->get_in_buffer();
  auto &out_buf = conn->get_out_buffer();
  size_t ptr = 0;

  while (ptr < in_buf.size()) {
    try {
      RespValue value = RespParser::parse(in_buf, ptr);
      Command command = CommandParser::parse_from_resp(value);
      RespValue res = CommandExecuter::execute(command, storage);
      RespWriter::write(out_buf, res);
    } catch (RespNotEnoughException &) {
      break;
    }
  }

  if (ptr > 0) {
    buf_consume(in_buf, ptr);
    conn->set_want_write(true);
  }
}

void TcpServer::accept_connection() {
  while (true) {
    sockaddr_in connection_addr{};
    socklen_t connection_len = sizeof(connection_addr);

    int connection_fd =
        accept(listen_fd, (sockaddr *)&connection_addr, &connection_len);

    if (connection_fd >= 0) {
      Socket client_socket(connection_fd);
      client_socket.set_nonblocking();
      poller.add_poll_interest(PollInterest(client_socket.get_fd(),
                                            POLL_READABLE, PollTrigger::Edge));
      int fd = client_socket.get_fd();
      TcpConnection *tcp_connection =
          new TcpConnection(std::move(client_socket), connection_addr);
      connections[fd] = tcp_connection;
      Logger::info("New connection from: " +
                   sockaddr_to_string(connection_addr));
      continue;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }

    if (errno == EINTR) {
      continue;
    }

    Logger::error(
        std::system_error(errno, std::generic_category(), "accept failed")
            .what());
    break;
  }
}

void TcpServer::close_connection(int fd) {
  poller.delete_poll_interest(fd);

  auto it = connections.find(fd);
  if (it == connections.end()) {
    return;
  }
  TcpConnection *conn = it->second;
  Logger::info("Client disconnected: " +
               sockaddr_to_string(conn->get_peer_addr()));
  delete conn;
  connections.erase(it);
}