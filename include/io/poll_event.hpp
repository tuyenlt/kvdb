#pragma once

#include <cstdint>

constexpr uint32_t POLL_NONE = 0;
constexpr uint32_t POLL_READABLE = 1 << 0;
constexpr uint32_t POLL_WRITEABLE = 1 << 1;
constexpr uint32_t POLL_ERROR = 1 << 2;
constexpr uint32_t POLL_HANGUP = 1 << 3;
constexpr uint32_t POLL_SHUTDOWN = 1 << 4;

constexpr uint32_t POLL_WANT_READ = 1 << 0;
constexpr uint32_t POLL_WANT_WRITE = 1 << 1;
constexpr uint32_t POLL_WANT_CLOSE = 1 << 2;

enum class PollTrigger { Level, Edge };

class PollEvent {
public:
  PollEvent() {}
  PollEvent(int fd, int mask) : fd(fd), mask(mask) {}
  int fd;
  uint32_t mask;
};

class PollInterest {
public:
  PollInterest(int fd, uint32_t mask, PollTrigger trigger)
      : fd(fd), mask(mask), trigger(trigger) {};
  int fd;
  uint32_t mask;
  PollTrigger trigger;
};