#pragma once
#include <string>

enum class ValueType { STRING, LIST, HASH, SET, ZSET, STREAM };

class Value {
public:
  virtual ~Value() = default;
  virtual ValueType type() const = 0;
};

class StringValue : public Value {

public:
  std::string value;
  explicit StringValue(const std::string_view value)
      : value(std::string(value)) {}
  ValueType type() const override { return ValueType::STRING; }
};
