#pragma once

#include <optional>
#include <string>
#include <unordered_map>
class Storage {
  std::unordered_map<std::string, std::string> map;

public:
  Storage();
  std::optional<std::string> get(std::string &key);
  void set(std::string &key, std::string &value);
};