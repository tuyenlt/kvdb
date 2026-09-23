#pragma once

#include "command/command.hpp"
#include "net/protocol.hpp"
#include <string>
#include <sys/types.h>

class CommandParserException : public std::exception {
private:
  std::string message_;

public:
  explicit CommandParserException(const std::string &message)
      : message_(message) {}

  const char *what() const noexcept override { return message_.c_str(); }
};

class CommandParser {
public:
  static Command parse_from_resp(RespValue &val);
};