#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ─── RESP type sigils ────────────────────────────────────────────────────────
constexpr uint8_t RESP_SIMPLE_STR = '+';
constexpr uint8_t RESP_ERROR = '-';
constexpr uint8_t RESP_INTEGER = ':';
constexpr uint8_t RESP_BULK_STR = '$';
constexpr uint8_t RESP_ARRAYS = '*';
constexpr uint8_t RESP_NULL = '_';   // RESP3
constexpr uint8_t RESP_DOUBLE = ','; // RESP3

// ─── Exceptions ──────────────────────────────────────────────────────────────

/// Thrown when the data in the buffer is malformed / violates the RESP spec.
class RespParseException : public std::exception {
  std::string message_;

public:
  explicit RespParseException(const std::string &message) : message_(message) {}
  const char *what() const noexcept override { return message_.c_str(); }
};

/// Thrown when the buffer holds a valid but incomplete RESP message.
/// The caller should buffer more data and retry.
class RespNotEnoughException : public std::exception {
  std::string message_;

public:
  explicit RespNotEnoughException(const std::string &message)
      : message_(message) {}
  const char *what() const noexcept override { return message_.c_str(); }
};

// ─── RespValue ───────────────────────────────────────────────────────────────

enum class RespType {
  SimpleString,
  Error,
  Integer,
  BulkString,
  Array,
  Null,
  Double,
};

/**
 * A tagged-union representing any parsed RESP value.
 *
 * Ownership model:
 *   - SimpleString / Error  → std::string  (owned copy)
 *   - BulkString            → std::optional<std::string>  (nullopt = null bulk)
 *   - Integer               → int64_t
 *   - Double                → double
 *   - Array                 → std::optional<std::vector<RespValue>>
 *                             (nullopt = null array)
 *   - Null                  → (no payload)
 */
struct RespValue {
  using Array = std::vector<RespValue>;

  RespType type;

  // Only one of these is active at a time (chosen by `type`).
  std::string str; // SimpleString, Error, BulkString (non-null)
  int64_t integer{0};
  double dbl{0.0};
  bool is_null{false};          // null bulk / null array
  std::vector<RespValue> array; // Array elements

  // ── Factories ─────────────────────────────────────────────────────────────
  static RespValue make_simple_string(std::string s);
  static RespValue make_error(std::string s);
  static RespValue make_integer(int64_t v);
  static RespValue make_bulk_string(std::string s);
  static RespValue make_null_bulk();
  static RespValue make_array(std::vector<RespValue> elems);
  static RespValue make_null_array();
  static RespValue make_null();
  static RespValue make_double(double v);
};

// ─── RespParser ──────────────────────────────────────────────────────────────

/**
 * Stateless, streaming RESP parser.
 *
 * Every method advances `ptr` past the bytes it consumed.
 * Throws RespNotEnoughException if more data is needed (ptr is NOT advanced).
 * Throws RespParseException     if the data is malformed.
 */
class RespParser {
  // ── Internal helpers ──────────────────────────────────────────────────────
  static size_t find_crlf(const std::vector<uint8_t> &buf, size_t ptr);
  static int64_t read_integer_line(const std::vector<uint8_t> &buf,
                                   size_t &ptr);
  static double read_double_line(const std::vector<uint8_t> &buf, size_t &ptr);

public:
  /**
   * Parse the next complete RESP value starting at buf[ptr].
   * On success, ptr points past the last consumed byte.
   */
  static RespValue parse(const std::vector<uint8_t> &buf, size_t &ptr);

  // ── Low-level type parsers (also usable individually) ────────────────────
  static std::string parse_simple_string(const std::vector<uint8_t> &buf,
                                         size_t &ptr);
  static std::string parse_error(const std::vector<uint8_t> &buf, size_t &ptr);
  static int64_t parse_integer(const std::vector<uint8_t> &buf, size_t &ptr);
  static std::optional<std::string>
  parse_bulk_string(const std::vector<uint8_t> &buf, size_t &ptr);
  static int64_t parse_array_len(const std::vector<uint8_t> &buf, size_t &ptr);
  static double parse_double(const std::vector<uint8_t> &buf, size_t &ptr);
};

// ─── RespWriter ──────────────────────────────────────────────────────────────

/**
 * Serialises RespValue objects into wire-format bytes.
 *
 * All methods append to `out`; they never clear it.
 */
class RespWriter {
public:
  static void write(std::vector<uint8_t> &out, const RespValue &val);

  static void write_simple_string(std::vector<uint8_t> &out,
                                  const std::string &s);
  static void write_error(std::vector<uint8_t> &out, const std::string &s);
  static void write_integer(std::vector<uint8_t> &out, int64_t v);
  static void write_bulk_string(std::vector<uint8_t> &out, const std::string &s);
  static void write_null_bulk(std::vector<uint8_t> &out);
  static void write_array(std::vector<uint8_t> &out,
                          const std::vector<RespValue> &elems);
  static void write_null_array(std::vector<uint8_t> &out);
  static void write_null(std::vector<uint8_t> &out);
  static void write_double(std::vector<uint8_t> &out, double v);
};