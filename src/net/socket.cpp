#include "net/socket.hpp"
#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

Socket::Socket(int fd) noexcept : fd(fd) {}

Socket::~Socket() { close(); }

Socket::Socket(Socket &&other) noexcept : fd(other.fd) { other.fd = -1; }

Socket &Socket::operator=(Socket &&other) noexcept {
  if (this != &other) {
    close();
    fd = other.fd;
    other.fd = -1;
  }
  return *this;
}

ssize_t Socket::read(void *buf, size_t count) {
  if (fd < 0) {
    errno = EBADF;
    return -1;
  }
  return ::read(fd, buf, count);
}

ssize_t Socket::write(const void *buf, size_t count) {
  if (fd < 0) {
    errno = EBADF;
    return -1;
  }
  return ::write(fd, buf, count);
}

void Socket::set_nonblocking() {
  if (fd >= 0) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      throw std::system_error(errno, std::generic_category(),
                              "fcntl F_GETFL failed");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      throw std::system_error(errno, std::generic_category(),
                              "fcntl F_SETFL failed");
    }
  } else {
    throw std::system_error(
        std::make_error_code(std::errc::bad_file_descriptor),
        "Socket::set_nonblocking called on invalid socket");
  }
}

void Socket::close() noexcept {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

int Socket::release() noexcept {
  int old_fd = fd;
  fd = -1;
  return old_fd;
}

void Socket::reset(int new_fd) noexcept {
  if (fd != new_fd) {
    close();
    fd = new_fd;
  }
}
