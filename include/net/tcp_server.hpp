#pragma once

#include "io/epoll.hpp"
#include "net/tcp_connection.hpp"
#include <functional>
#include <netinet/in.h>
#include <unordered_map>

#define SOCK_MAX_CONN 10
#define MAX_POLL_EVENTS 40

class TcpServer {
  int listen_fd;
  sockaddr_in listen_addr;
  std::unordered_map<int, TcpConnection *> connections;
  Epoll poller;
  void accept_connection();
  void close_connection(int fd);
  std::function<void(TcpConnection *conn)> read_callback_handler;

public:
  TcpServer();
  TcpServer(const char *addr, int port);
  void start();
  void set_read_callback_handler(std::function<void(TcpConnection *conn)> cb) {
    read_callback_handler = cb;
  }
};
