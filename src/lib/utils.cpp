#include "lib/utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <vector>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
  if (!data || len == 0) {
    return;
  }
  buf.insert(buf.end(), data, data + len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t n) {
  if (n >= buf.size()) {
    buf.clear();
  } else {
    buf.erase(buf.begin(), buf.begin() + n);
  }
}

std::string sockaddr_to_string(const sockaddr_in &addr) {
  char ip[INET_ADDRSTRLEN];

  if (inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
    throw std::system_error(errno, std::generic_category(), "inet_ntop failed");
  }

  return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}