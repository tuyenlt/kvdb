#include "storage/strorage.hpp"
#include <optional>
#include <string>

Storage::Storage() {}

void Storage::set(std::string &key, std::string &val) { map[key] = val; }

std::optional<std::string> Storage::get(std::string &key) {
  if (map.find(key) == map.end()) {
    return std::nullopt;
  }
  return map[key];
}
