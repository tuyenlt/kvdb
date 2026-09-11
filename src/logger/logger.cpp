#include "logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

static const char *level_to_string(LogLevel level) {
  switch (level) {
  case LogLevel::DEBUG:
    return "DEBUG";

  case LogLevel::INFO:
    return "INFO";

  case LogLevel::WARN:
    return "WARN";

  case LogLevel::ERROR:
    return "ERROR";
  }

  return "UNKNOWN";
}

void Logger::log(LogLevel level, std::string_view message) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::tm tm{};

  localtime_r(&time, &tm);

  std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ["
            << level_to_string(level) << "] " << message << '\n';
}

void Logger::debug(std::string_view message) { log(LogLevel::DEBUG, message); }

void Logger::info(std::string_view message) { log(LogLevel::INFO, message); }

void Logger::warn(std::string_view message) { log(LogLevel::WARN, message); }

void Logger::error(std::string_view message) { log(LogLevel::ERROR, message); }