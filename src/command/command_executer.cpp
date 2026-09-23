#include "command/command_executer.hpp"
#include "net/protocol.hpp"

RespValue CommandExecuter::execute(Command &command, Storage &storage) {
  if (command.argv.empty()) {
    return RespValue::make_error("Empty command");
  }
  std::string command_type = command.type;
  if (command_type == "SET") {
    return execute_set_command(command, storage);
  } else if (command_type == "GET") {
    return execute_get_command(command, storage);
    //   } else if (command_type == "DEL") {
    //     return execute_del_command(command);
    //   } else if (command_type == "EXISTS") {
    //     return execute_exists_command(command);
  } else {
    return execute_unknown_command(command, storage);
  }
}

RespValue CommandExecuter::execute_set_command(Command &command,
                                               Storage &storage) {
  if (command.argv.size() != 3) {
    return RespValue::make_error("Invalid command");
  }
  storage.set(command.argv[1].value(), command.argv[2].value());
  return RespValue::make_simple_string("OK");
}

RespValue CommandExecuter::execute_get_command(Command &command,
                                               Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  std::optional<std::string> value = storage.get(command.argv[1].value());
  if (value.has_value()) {
    return RespValue::make_bulk_string(value.value());
  } else {
    return RespValue::make_null_bulk();
  }
}

RespValue CommandExecuter::execute_unknown_command(Command &command,
                                                   Storage &storage) {
  return RespValue::make_error("Unknown command: " + command.type);
}