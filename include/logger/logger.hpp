#pragma once

#include <string_view>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  static void log(LogLevel level, std::string_view message);

  static void debug(std::string_view message);
  static void info(std::string_view message);
  static void warn(std::string_view message);
  static void error(std::string_view message);
};