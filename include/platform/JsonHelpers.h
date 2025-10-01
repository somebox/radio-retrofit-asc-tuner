#pragma once

#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace events {
namespace json {

struct Field {
  Field(std::string key_in, std::string value_in, bool enabled_in)
      : key(std::move(key_in)), value(std::move(value_in)), enabled(enabled_in) {}

  std::string key;
  std::string value;
  bool enabled{true};
};

inline Field field(std::string_view key, std::string value, bool enabled = true) {
  return Field(std::string(key), std::move(value), enabled);
}

inline std::string escape(std::string_view input) {
  std::string escaped;
  escaped.reserve(input.size() + 8);
  for (char c : input) {
    switch (c) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          // Skip control characters not expected in payloads.
        } else {
          escaped.push_back(c);
        }
        break;
    }
  }
  return escaped;
}

inline std::string string_value(std::string_view input) {
  std::string quoted;
  quoted.reserve(input.size() + 2);
  quoted.push_back('"');
  quoted.append(escape(input));
  quoted.push_back('"');
  return quoted;
}

inline std::string boolean(bool value) {
  return value ? "true" : "false";
}

template <typename Integer,
          typename = std::enable_if_t<std::is_integral<Integer>::value &&
                                      !std::is_same<Integer, bool>::value>>
inline std::string number(Integer value) {
  return std::to_string(value);
}

inline std::string number(double value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%g", value);
  return std::string(buffer);
}

inline Field string_field(std::string_view key, std::string_view value, bool enabled = true) {
  return field(key, string_value(value), enabled);
}

inline Field string_field(std::string_view key, const char* value, bool enabled = true) {
  if (!value || !enabled) {
    return field(key, std::string{}, false);
  }
  if (value[0] == '\0') {
    return field(key, std::string{}, false);
  }
  return string_field(key, std::string_view(value));
}

template <typename Integer>
inline Field number_field(std::string_view key, Integer value, bool enabled = true) {
  return field(key, number(value), enabled);
}

inline Field number_field(std::string_view key, double value, bool enabled = true) {
  return field(key, number(value), enabled);
}

inline Field boolean_field(std::string_view key, bool value, bool enabled = true) {
  return field(key, boolean(value), enabled);
}

template <typename Iterator>
inline std::string object_from_range(Iterator begin, Iterator end) {
  std::string result;
  result.push_back('{');
  bool first = true;
  for (auto it = begin; it != end; ++it) {
    if (!it->enabled) {
      continue;
    }
    if (!first) {
      result.push_back(',');
    }
    result.push_back('"');
    result.append(it->key);
    result.append("\":");
    result.append(it->value);
    first = false;
  }
  result.push_back('}');
  return result;
}

inline std::string object(std::initializer_list<Field> fields) {
  return object_from_range(fields.begin(), fields.end());
}

inline std::string object(const std::vector<Field>& fields) {
  return object_from_range(fields.begin(), fields.end());
}

}  // namespace json
}  // namespace events


