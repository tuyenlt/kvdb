#pragma once

#include "storage/value.hpp"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

struct StringHash {
  using is_transparent = void;
  size_t operator()(std::string_view sv) const noexcept {
    return std::hash<std::string_view>{}(sv);
  }
};

class Storage {
  std::unordered_map<std::string, std::unique_ptr<Value>, StringHash,
                     std::equal_to<>>
      map;

public:
  Storage();
  const Value *get(std::string_view key) const;
  void set(std::string_view key, std::unique_ptr<Value> value);
  void del(std::string_view key);
  bool exists(std::string_view key);
  void save_snapshot();
  void load_snapshot();
};