#include "command/command_parser.hpp"
#include "command/command.hpp"
#include "net/protocol.hpp"
#include <vector>

Command CommandParser::parse_from_resp(RespValue &val) {
  if (val.type != RespType::Array) {
    throw CommandParserException("command must start with array");
  }
  if (val.array.empty()) {
    throw CommandParserException("empty command array");
  }
  Command command;
  for (int i = 0; i < val.array.size(); i++) {
    if (val.array[i].type != RespType::BulkString) {
      throw CommandParserException("command must be bulk string");
    }
    if (i == 0) {
      command.type = val.array[i].str;
    }
    command.argv.push_back(val.array[i].str);
  }
  return command;
}