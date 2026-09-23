#pragma once

#include <optional>
#include <string>
#include <vector>

struct Command {
  std::string type;
  std::vector<std::optional<std::string>> argv;
};
