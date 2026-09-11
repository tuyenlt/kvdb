#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <vector>

void set_nonblocking_fd(int fd);
void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t n);
std::string sockaddr_to_string(const sockaddr_in &addr);