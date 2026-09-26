#include "command/command_executer.hpp"
#include "net/protocol.hpp"
#include "storage/strorage.hpp"
#include "storage/value.hpp"
#include <stdexcept>

RespValue CommandExecuter::execute(Command &command, Storage &storage) {
  if (command.argv.empty()) {
    return RespValue::make_error("Empty command");
  }
  std::string_view command_type = command.type;
  if (command_type == "SET") {
    return execute_set_command(command, storage);
  } else if (command_type == "GET") {
    return execute_get_command(command, storage);
  } else if (command_type == "DEL") {
    return execute_del_command(command, storage);
  } else if (command_type == "EXISTS") {
    return execute_exists_command(command, storage);
  } else if (command_type == "INCR") {
    return execute_incr_command(command, storage);
  } else if (command_type == "DECR") {
    return execute_decr_command(command, storage);
  } else {
    return execute_unknown_command(command, storage);
  }
}

RespValue CommandExecuter::execute_unknown_command(Command &command,
                                                   Storage &storage) {
  return RespValue::make_error("ERR unknown command");
}

RespValue CommandExecuter::execute_set_command(Command &command,
                                               Storage &storage) {
  if (command.argv.size() != 3) {
    return RespValue::make_error("Invalid command");
  }
  storage.set(command.argv[1], std::make_unique<StringValue>(command.argv[2]));
  return RespValue::make_simple_string("OK");
}

RespValue CommandExecuter::execute_get_command(Command &command,
                                               Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  const Value *value = storage.get(command.argv[1]);
  if (value == nullptr) {
    return RespValue::make_null_bulk();
  }
  if (value->type() != ValueType::STRING) {
    return RespValue::make_error("Wrong type");
  }

  const auto *string_value = static_cast<const StringValue *>(value);

  return RespValue::make_bulk_string(string_value->value);
}

RespValue CommandExecuter::execute_del_command(Command &command,
                                               Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  storage.del(command.argv[1]);
  return RespValue::make_integer(1);
}

RespValue CommandExecuter::execute_exists_command(Command &command,
                                                  Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  return RespValue::make_integer(storage.exists(command.argv[1]));
}

RespValue CommandExecuter::execute_incr_command(Command &command,
                                                Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  const Value *value = storage.get(command.argv[1]);
  if (value == nullptr) {
    return RespValue::make_null_bulk();
  }
  if (value->type() != ValueType::STRING) {
    return RespValue::make_error("Wrong type");
  }

  const auto *string_value = static_cast<const StringValue *>(value);

  try {
    long long number = stoll(string_value->value);
    number++;
    storage.set(command.argv[1],
                std::make_unique<StringValue>(std::to_string(number)));
    return RespValue::make_integer(number);
  } catch (const std::invalid_argument &e) {
    return RespValue::make_error("Value is not a number");
  } catch (const std::out_of_range &e) {
    return RespValue::make_error("Value is out of range");
  }
}

RespValue CommandExecuter::execute_decr_command(Command &command,
                                                Storage &storage) {
  if (command.argv.size() != 2) {
    return RespValue::make_error("Invalid command");
  }
  const Value *value = storage.get(command.argv[1]);
  if (value == nullptr) {
    return RespValue::make_null_bulk();
  }
  if (value->type() != ValueType::STRING) {
    return RespValue::make_error("Wrong type");
  }

  const auto *string_value = static_cast<const StringValue *>(value);

  try {
    long long number = stoll(string_value->value);
    number--;
    storage.set(command.argv[1],
                std::make_unique<StringValue>(std::to_string(number)));
    return RespValue::make_integer(number);
  } catch (const std::invalid_argument &e) {
    return RespValue::make_error("Value is not a number");
  } catch (const std::out_of_range &e) {
    return RespValue::make_error("Value is out of range");
  }
}