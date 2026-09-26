#pragma once

#include <string_view>
#include <vector>

struct Command {
  std::string_view type;
  std::vector<std::string_view> argv;
};
