#include "net/tcp_connection.hpp"
#include "io/poll_event.hpp"
#include "lib/utils.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>
#include <vector>

TcpConnection::TcpConnection(Socket socket, sockaddr_in peer_addr)
    : socket(std::move(socket)), peer_addr(peer_addr) {}

TcpConnection::TcpConnection(int fd, sockaddr_in peer_addr)
    : socket(fd), peer_addr(peer_addr) {}

TcpIOResult TcpConnection::read_buffer() {
  uint8_t buffer[64 * 1024];
  bool data_read = false;

  while (true) {
    ssize_t len_read = socket.read(buffer, sizeof(buffer));

    if (len_read > 0) {
      buf_append(in_buffer, buffer, len_read);
      data_read = true;
      continue;
    }

    if (len_read == 0) {
      return TcpIOResult::CLOSED; // Connection closed by client
    }

    if (len_read == -1) {
      if (errno == EINTR) {
        continue;
      }

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return data_read ? TcpIOResult::DONE : TcpIOResult::WBLOCK;
      }

      return TcpIOResult::ERROR;
    }
  }
}

TcpIOResult TcpConnection::write_buffer() {
  while (!out_buffer.empty()) {
    ssize_t len_write = socket.write(out_buffer.data(), out_buffer.size());

    if (len_write > 0) {
      buf_consume(out_buffer, len_write);
      continue;
    }

    if (len_write == 0) {
      interest_mask |= POLL_WANT_WRITE;
      return TcpIOResult::WBLOCK;
    }

    if (len_write == -1) {
      if (errno == EINTR) {
        continue;
      }

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        interest_mask |= POLL_WANT_WRITE;
        return TcpIOResult::WBLOCK;
      }

      if (errno == EPIPE || errno == ECONNRESET) {
        return TcpIOResult::CLOSED;
      }

      return TcpIOResult::ERROR;
    }
  }

  interest_mask &= ~POLL_WANT_WRITE;
  return TcpIOResult::DONE;
}

void TcpConnection::add_write_buffer(const std::vector<uint8_t> &buffer) {
  buf_append(out_buffer, buffer.data(), buffer.size());
}

void TcpConnection::set_interest_mask(uint32_t mask) { interest_mask = mask; };

uint32_t TcpConnection::get_interest_mask() { return interest_mask; };

sockaddr_in TcpConnection::get_peer_addr() { return peer_addr; };