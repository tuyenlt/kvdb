#include "net/protocol.hpp"

#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════════
// RespValue factories
// ═══════════════════════════════════════════════════════════════════════════════

RespValue RespValue::make_simple_string(std::string_view s) {
  RespValue v;
  v.type = RespType::SimpleString;
  v.str = s;
  return v;
}

RespValue RespValue::make_error(std::string_view s) {
  RespValue v;
  v.type = RespType::Error;
  v.str = s;
  return v;
}

RespValue RespValue::make_integer(int64_t i) {
  RespValue v;
  v.type = RespType::Integer;
  v.integer = i;
  return v;
}

RespValue RespValue::make_bulk_string(std::string_view s) {
  RespValue v;
  v.type = RespType::BulkString;
  v.str = s;
  return v;
}

RespValue RespValue::make_null_bulk() {
  RespValue v;
  v.type = RespType::BulkString;
  v.is_null = true;
  return v;
}

RespValue RespValue::make_array(std::vector<RespValue> elems) {
  RespValue v;
  v.type = RespType::Array;
  v.array = std::move(elems);
  return v;
}

RespValue RespValue::make_null_array() {
  RespValue v;
  v.type = RespType::Array;
  v.is_null = true;
  return v;
}

RespValue RespValue::make_null() {
  RespValue v;
  v.type = RespType::Null;
  v.is_null = true;
  return v;
}

RespValue RespValue::make_double(double d) {
  RespValue v;
  v.type = RespType::Double;
  v.dbl = d;
  return v;
}

// ═══════════════════════════════════════════════════════════════════════════════
// RespParser  –  internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Locate the position of '\r' in buf[ptr..] such that buf[pos+1] == '\n'.
 * Returns the position of '\r'.
 * Throws RespNotEnoughException if the CRLF terminator is not yet available.
 * Throws RespParseException     if '\r' is found but not followed by '\n'.
 */
size_t RespParser::find_crlf(const std::vector<uint8_t> &buf, size_t ptr) {
  for (size_t i = ptr; i < buf.size(); ++i) {
    if (buf[i] == '\r') {
      if (i + 1 >= buf.size()) {
        throw RespNotEnoughException("not enough data: incomplete CRLF");
      }
      if (buf[i + 1] != '\n') {
        throw RespParseException("invalid CRLF: '\\r' not followed by '\\n'");
      }
      return i;
    }
  }
  throw RespNotEnoughException("not enough data: CRLF not found");
}

/**
 * Read a decimal integer from buf[ptr] up to the next CRLF and advance ptr.
 * Does NOT skip the initial type-sigil byte; the caller must do that.
 */
int64_t RespParser::read_integer_line(const std::vector<uint8_t> &buf,
                                      size_t &ptr) {
  size_t crlf = find_crlf(buf, ptr);

  if (crlf == ptr) {
    throw RespParseException("empty integer field");
  }

  const char *begin = reinterpret_cast<const char *>(buf.data() + ptr);
  const char *end = reinterpret_cast<const char *>(buf.data() + crlf);

  int64_t val = 0;
  auto [parse_end, ec] = std::from_chars(begin, end, val);

  if (ec != std::errc{} || parse_end != end) {
    throw RespParseException("invalid integer value");
  }

  ptr = crlf + 2; // skip \r\n
  return val;
}

/**
 * Read a double from buf[ptr] up to the next CRLF and advance ptr.
 * Handles the special RESP3 values "inf", "-inf", "nan".
 */
double RespParser::read_double_line(const std::vector<uint8_t> &buf,
                                    size_t &ptr) {
  size_t crlf = find_crlf(buf, ptr);

  if (crlf == ptr) {
    throw RespParseException("empty double field");
  }

  std::string token(reinterpret_cast<const char *>(buf.data() + ptr),
                    crlf - ptr);
  ptr = crlf + 2;

  if (token == "inf")
    return std::numeric_limits<double>::infinity();
  if (token == "-inf")
    return -std::numeric_limits<double>::infinity();
  if (token == "nan")
    return std::numeric_limits<double>::quiet_NaN();

  double val = 0.0;
  const char *begin = token.data();
  const char *end = token.data() + token.size();
  auto [parse_end, ec] = std::from_chars(begin, end, val);

  if (ec != std::errc{} || parse_end != end) {
    throw RespParseException("invalid double value: " + token);
  }

  return val;
}

// ═══════════════════════════════════════════════════════════════════════════════
// RespParser  –  public API
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Parse a Simple String: +<data>\r\n
 * ptr must point at the '+' sigil.
 */
std::string_view
RespParser::parse_simple_string(const std::vector<uint8_t> &buf, size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_SIMPLE_STR)
    throw RespParseException("expected '+' for Simple String");

  ++ptr; // skip '+'
  size_t crlf = find_crlf(buf, ptr);

  std::string_view result(reinterpret_cast<const char *>(buf.data() + ptr),
                          crlf - ptr);
  ptr = crlf + 2;
  return result;
}

/**
 * Parse an Error: -<message>\r\n
 * ptr must point at the '-' sigil.
 */
std::string_view RespParser::parse_error(const std::vector<uint8_t> &buf,
                                         size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_ERROR)
    throw RespParseException("expected '-' for Error");

  ++ptr; // skip '-'
  size_t crlf = find_crlf(buf, ptr);

  std::string_view result(reinterpret_cast<const char *>(buf.data() + ptr),
                          crlf - ptr);
  ptr = crlf + 2;
  return result;
}

/**
 * Parse an Integer: :<number>\r\n
 * ptr must point at the ':' sigil.
 */
int64_t RespParser::parse_integer(const std::vector<uint8_t> &buf,
                                  size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_INTEGER)
    throw RespParseException("expected ':' for Integer");

  ++ptr; // skip ':'
  return read_integer_line(buf, ptr);
}

/**
 * Parse a Bulk String: $<len>\r\n<data>\r\n  or  $-1\r\n (null bulk)
 * ptr must point at the '$' sigil.
 * Returns std::nullopt for a null bulk string.
 */
std::optional<std::string_view>
RespParser::parse_bulk_string(const std::vector<uint8_t> &buf, size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_BULK_STR)
    throw RespParseException("expected '$' for Bulk String");

  size_t cursor = ptr;
  ++cursor; // skip '$'

  int64_t len = read_integer_line(buf, cursor);

  if (len == -1) {
    ptr = cursor;
    return std::nullopt; // null bulk string
  }
  if (len < -1) {
    throw RespParseException("invalid bulk string length: " +
                             std::to_string(len));
  }

  auto payload_len = static_cast<size_t>(len);

  if (cursor + payload_len + 2 > buf.size()) {
    throw RespNotEnoughException("not enough data: bulk payload incomplete");
  }
  if (buf[cursor + payload_len] != '\r' ||
      buf[cursor + payload_len + 1] != '\n') {
    throw RespParseException("bulk string missing trailing CRLF");
  }

  std::string_view result(reinterpret_cast<const char *>(buf.data() + cursor),
                          payload_len);
  ptr = cursor + payload_len + 2;
  return result;
}

/**
 * Parse the array length header only: *<count>\r\n
 * Returns the count (-1 = null array).
 * ptr must point at the '*' sigil.
 */
int64_t RespParser::parse_array_len(const std::vector<uint8_t> &buf,
                                    size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_ARRAYS)
    throw RespParseException("expected '*' for Array");

  ++ptr; // skip '*'
  return read_integer_line(buf, ptr);
}

/**
 * Parse a Double (RESP3): ,<value>\r\n
 * ptr must point at the ',' sigil.
 */
double RespParser::parse_double(const std::vector<uint8_t> &buf, size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");
  if (buf[ptr] != RESP_DOUBLE)
    throw RespParseException("expected ',' for Double");

  ++ptr; // skip ','
  return read_double_line(buf, ptr);
}

/**
 * Dispatch-parse any RESP value.
 * Recursively parses array elements.
 */
RespValue RespParser::parse(const std::vector<uint8_t> &buf, size_t &ptr) {
  if (ptr >= buf.size())
    throw RespNotEnoughException("not enough data");

  switch (buf[ptr]) {
  case RESP_SIMPLE_STR:
    return RespValue::make_simple_string(parse_simple_string(buf, ptr));

  case RESP_ERROR:
    return RespValue::make_error(parse_error(buf, ptr));

  case RESP_INTEGER:
    return RespValue::make_integer(parse_integer(buf, ptr));

  case RESP_BULK_STR: {
    auto s = parse_bulk_string(buf, ptr);
    if (!s)
      return RespValue::make_null_bulk();
    return RespValue::make_bulk_string(*s);
  }

  case RESP_ARRAYS: {
    int64_t len = parse_array_len(buf, ptr);
    if (len == -1)
      return RespValue::make_null_array();
    if (len < -1)
      throw RespParseException("invalid array length");

    std::vector<RespValue> elems;
    elems.reserve(static_cast<size_t>(len));
    for (int64_t i = 0; i < len; ++i) {
      elems.push_back(parse(buf, ptr));
    }
    return RespValue::make_array(std::move(elems));
  }

  case RESP_NULL: {
    // RESP3 Null: _\r\n
    ++ptr;
    size_t crlf = find_crlf(buf, ptr);
    if (crlf != ptr)
      throw RespParseException("unexpected data after null sigil");
    ptr = crlf + 2;
    return RespValue::make_null();
  }

  case RESP_DOUBLE:
    return RespValue::make_double(parse_double(buf, ptr));

  default:
    throw RespParseException(std::string("unknown RESP type byte: 0x") + [&]() {
      char buf2[3];
      snprintf(buf2, sizeof(buf2), "%02x", buf[ptr]);
      return std::string(buf2);
    }());
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// RespWriter
// ═══════════════════════════════════════════════════════════════════════════════

static void append(std::vector<uint8_t> &out, std::string_view sv) {
  out.insert(out.end(), sv.begin(), sv.end());
}

static void append_crlf(std::vector<uint8_t> &out) {
  out.push_back('\r');
  out.push_back('\n');
}

void RespWriter::write_simple_string(std::vector<uint8_t> &out,
                                     std::string_view s) {
  out.push_back(RESP_SIMPLE_STR);
  append(out, s);
  append_crlf(out);
}

void RespWriter::write_error(std::vector<uint8_t> &out, std::string_view s) {
  out.push_back(RESP_ERROR);
  append(out, s);
  append_crlf(out);
}

void RespWriter::write_integer(std::vector<uint8_t> &out, int64_t v) {
  out.push_back(RESP_INTEGER);
  append(out, std::to_string(v));
  append_crlf(out);
}

void RespWriter::write_bulk_string(std::vector<uint8_t> &out,
                                   std::string_view s) {
  out.push_back(RESP_BULK_STR);
  append(out, std::to_string(s.size()));
  append_crlf(out);
  append(out, s);
  append_crlf(out);
}

void RespWriter::write_null_bulk(std::vector<uint8_t> &out) {
  // RESP2 null bulk: $-1\r\n
  append(out, "$-1\r\n");
}

void RespWriter::write_null_array(std::vector<uint8_t> &out) {
  // RESP2 null array: *-1\r\n
  append(out, "*-1\r\n");
}

void RespWriter::write_null(std::vector<uint8_t> &out) {
  // RESP3 null: _\r\n
  append(out, "_\r\n");
}

void RespWriter::write_array(std::vector<uint8_t> &out,
                             const std::vector<RespValue> &elems) {
  out.push_back(RESP_ARRAYS);
  append(out, std::to_string(elems.size()));
  append_crlf(out);
  for (const auto &elem : elems) {
    write(out, elem);
  }
}

void RespWriter::write_double(std::vector<uint8_t> &out, double v) {
  out.push_back(RESP_DOUBLE);
  // Use a sufficiently precise representation
  char buf[64];
  if (std::isinf(v)) {
    snprintf(buf, sizeof(buf), v > 0 ? "inf" : "-inf");
  } else if (std::isnan(v)) {
    snprintf(buf, sizeof(buf), "nan");
  } else {
    snprintf(buf, sizeof(buf), "%.17g", v);
  }
  append(out, buf);
  append_crlf(out);
}

void RespWriter::write(std::vector<uint8_t> &out, const RespValue &val) {
  switch (val.type) {
  case RespType::SimpleString:
    write_simple_string(out, val.str);
    break;
  case RespType::Error:
    write_error(out, val.str);
    break;
  case RespType::Integer:
    write_integer(out, val.integer);
    break;
  case RespType::BulkString:
    if (val.is_null)
      write_null_bulk(out);
    else
      write_bulk_string(out, val.str);
    break;
  case RespType::Array:
    if (val.is_null)
      write_null_array(out);
    else
      write_array(out, val.array);
    break;
  case RespType::Null:
    write_null(out);
    break;
  case RespType::Double:
    write_double(out, val.dbl);
    break;
  }
}