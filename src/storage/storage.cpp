#include "storage/strorage.hpp"

Storage::Storage() {}

void Storage::set(std::string_view key, std::unique_ptr<Value> value) {
  auto it = map.find(key);
  if (it != map.end()) {
    it->second = std::move(value);
  } else {
    map.emplace(std::string(key), std::move(value));
  }
}

const Value *Storage::get(std::string_view key) const {
  auto it = map.find(key);
  if (it == map.end()) {
    return nullptr;
  }
  return it->second.get();
}

void Storage::del(std::string_view key) {
  auto it = map.find(key);
  if (it != map.end()) {
    map.erase(it);
  }
}

bool Storage::exists(std::string_view key) {
  return map.find(key) != map.end();
}
