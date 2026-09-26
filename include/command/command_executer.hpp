#pragma once

#include "command/command.hpp"
#include "net/protocol.hpp"
#include "storage/strorage.hpp"

class CommandExecuter {
public:
  static RespValue execute(Command &command, Storage &storage);
  static RespValue execute_unknown_command(Command &command, Storage &storage);
  static RespValue execute_set_command(Command &command, Storage &storage);
  static RespValue execute_get_command(Command &command, Storage &storage);
  static RespValue execute_del_command(Command &command, Storage &storage);
  static RespValue execute_exists_command(Command &command, Storage &storage);
  static RespValue execute_incr_command(Command &command, Storage &storage);
  static RespValue execute_decr_command(Command &command, Storage &storage);
};