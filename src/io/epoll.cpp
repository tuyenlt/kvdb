#include "io/epoll.hpp"
#include "io/poll_event.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

Epoll::Epoll() : Epoll(DEFAULT_POLL_EVENTS_NUM) {}

Epoll::Epoll(int max_poll_events) : max_poll_events(max_poll_events) {
  if (max_poll_events <= 0) {
    throw std::invalid_argument("max_poll_events must be positive");
  }

  efd = epoll_create1(0);
  if (efd == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "epoll_create1 failed");
  }

  events.resize(max_poll_events);
}

Epoll::~Epoll() {
  if (efd >= 0) {
    close(efd);
    efd = -1;
  }
}

Epoll::Epoll(Epoll &&other) noexcept
    : efd(other.efd),
      max_poll_events(other.max_poll_events),
      events(std::move(other.events)) {
  other.efd = -1;
  other.max_poll_events = 0;
}

Epoll &Epoll::operator=(Epoll &&other) noexcept {
  if (this != &other) {
    if (efd >= 0) {
      close(efd);
    }
    efd = other.efd;
    max_poll_events = other.max_poll_events;
    events = std::move(other.events);

    other.efd = -1;
    other.max_poll_events = 0;
  }
  return *this;
}

void Epoll::add_poll_interest(PollInterest p_interest) {
  if (efd < 0) {
    throw std::system_error(
        std::make_error_code(std::errc::bad_file_descriptor),
        "epoll instance is not valid");
  }

  epoll_event event{};
  if (p_interest.trigger == PollTrigger::Edge) {
    event.events |= EPOLLET;
  }
  if (p_interest.mask & POLL_WANT_READ) {
    event.events |= EPOLLIN;
  }
  if (p_interest.mask & POLL_WANT_WRITE) {
    event.events |= EPOLLOUT;
  }
  event.data.fd = p_interest.fd;

  if (epoll_ctl(efd, EPOLL_CTL_ADD, p_interest.fd, &event) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "epoll_ctl ADD failed");
  }
}

void Epoll::modify_poll_interest(PollInterest p_interest) {
  if (efd < 0) {
    throw std::system_error(
        std::make_error_code(std::errc::bad_file_descriptor),
        "epoll instance is not valid");
  }

  epoll_event event{};
  if (p_interest.trigger == PollTrigger::Edge) {
    event.events |= EPOLLET;
  }
  if (p_interest.mask & POLL_WANT_READ) {
    event.events |= EPOLLIN;
  }
  if (p_interest.mask & POLL_WANT_WRITE) {
    event.events |= EPOLLOUT;
  }
  event.data.fd = p_interest.fd;

  if (epoll_ctl(efd, EPOLL_CTL_MOD, p_interest.fd, &event) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "epoll_ctl MOD failed");
  }
}

void Epoll::delete_poll_interest(int fd) {
  if (efd >= 0) {
    epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
  }
}

int Epoll::poll(std::vector<PollEvent> &p_events) {
  if (efd < 0) {
    throw std::system_error(
        std::make_error_code(std::errc::bad_file_descriptor),
        "epoll instance is not valid");
  }

  int event_count = epoll_wait(efd, events.data(), max_poll_events, -1);

  if (event_count == -1) {
    if (errno == EINTR) {
      return 0;
    }
    throw std::system_error(errno, std::generic_category(),
                            "epoll_wait failed");
  }

  if (p_events.size() < static_cast<size_t>(event_count)) {
    p_events.resize(event_count);
  }

  for (int i = 0; i < event_count; i++) {
    int mask = 0;
    if (events[i].events & EPOLLIN) {
      mask |= POLL_READABLE;
    }
    if (events[i].events & EPOLLOUT) {
      mask |= POLL_WRITEABLE;
    }
    if (events[i].events & EPOLLERR) {
      mask |= POLL_ERROR;
    }
    if (events[i].events & EPOLLHUP) {
      mask |= POLL_HANGUP;
    }
    if (events[i].events & EPOLLRDHUP) {
      mask |= POLL_SHUTDOWN;
    }
    p_events[i].mask = mask;
    p_events[i].fd = events[i].data.fd;
  }

  return event_count;
}