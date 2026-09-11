#pragma once

#include <cstddef>
#include <sys/types.h>

class Socket {
  int fd = -1;

public:
  Socket() noexcept = default;
  explicit Socket(int fd) noexcept;
  ~Socket();

  // Non-copyable (unique ownership of file descriptor)
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  // Move-constructible and move-assignable
  Socket(Socket &&other) noexcept;
  Socket &operator=(Socket &&other) noexcept;

  ssize_t read(void *buf, size_t count);
  ssize_t write(const void *buf, size_t count);

  void set_nonblocking();
  void close() noexcept;
  int release() noexcept;
  void reset(int new_fd = -1) noexcept;

  int get_fd() const noexcept { return fd; }
  bool is_valid() const noexcept { return fd >= 0; }
  explicit operator bool() const noexcept { return is_valid(); }
};
