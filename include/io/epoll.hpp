#pragma once

#include "io/poll_event.hpp"
#include <sys/epoll.h>
#include <vector>

#define DEFAULT_POLL_EVENTS_NUM 100

class Epoll {
  int efd = -1;
  int max_poll_events = 0;
  std::vector<epoll_event> events;

public:
  Epoll();
  explicit Epoll(int max_poll_events);
  ~Epoll();

  Epoll(const Epoll &) = delete;
  Epoll &operator=(const Epoll &) = delete;

  Epoll(Epoll &&other) noexcept;
  Epoll &operator=(Epoll &&other) noexcept;

  void add_poll_interest(PollInterest p_interest);
  void modify_poll_interest(PollInterest p_interest);
  void delete_poll_interest(int fd);
  int poll(std::vector<PollEvent> &p_events);

  int get_fd() const noexcept { return efd; }
  bool is_valid() const noexcept { return efd >= 0; }
};
