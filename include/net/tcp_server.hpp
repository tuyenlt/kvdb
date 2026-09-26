#pragma once

#include "io/epoll.hpp"
#include "net/tcp_connection.hpp"
#include "storage/strorage.hpp"
#include <netinet/in.h>
#include <unordered_map>

#define SOCK_MAX_CONN 100
#define MAX_POLL_EVENTS 500

class TcpServer {
  int listen_fd;
  sockaddr_in listen_addr;
  std::unordered_map<int, TcpConnection *> connections;
  Epoll poller;
  Storage storage;
  void accept_connection();
  void close_connection(int fd);
  void process_requests(TcpConnection *conn);

public:
  TcpServer();
  TcpServer(const char *addr, int port);
  void start();
};
