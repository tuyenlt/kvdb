#pragma once

#include "net/socket.hpp"
#include <cstdint>
#include <netinet/in.h>
#include <vector>

enum class TcpIOResult {
  DONE,
  ERROR,
  CLOSED,
  WBLOCK,
};

class TcpConnection {
  Socket socket;
  sockaddr_in peer_addr;
  std::vector<uint8_t> out_buffer;
  std::vector<uint8_t> in_buffer;

  bool want_read_  = true;
  bool want_write_ = false;
  bool want_close_ = false;

public:
  TcpConnection(Socket socket, sockaddr_in peer_addr);
  TcpConnection(int fd, sockaddr_in peer_addr);

  // Non-copyable due to unique socket ownership
  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  // Move-constructible and move-assignable
  TcpConnection(TcpConnection &&) noexcept = default;
  TcpConnection &operator=(TcpConnection &&) noexcept = default;

  TcpIOResult read_buffer();
  TcpIOResult write_buffer();
  void add_write_buffer(const std::vector<uint8_t> &buffer = {});

  // ── Intent flags ──────────────────────────────────────────────────────────
  void set_want_read(bool v)  { want_read_  = v; }
  void set_want_write(bool v) { want_write_ = v; }
  void set_want_close(bool v) { want_close_ = v; }

  bool want_read()  const { return want_read_;  }
  bool want_write() const { return want_write_; }
  bool want_close() const { return want_close_; }

  // ── Buffer & socket accessors ─────────────────────────────────────────────
  std::vector<uint8_t> &get_in_buffer()  { return in_buffer;  }
  std::vector<uint8_t> &get_out_buffer() { return out_buffer; }
  int get_fd() const { return socket.get_fd(); }
  Socket &get_socket() { return socket; }
  const Socket &get_socket() const { return socket; }
  sockaddr_in get_peer_addr();
};
