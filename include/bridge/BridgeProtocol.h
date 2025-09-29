#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace bridge {

// High-level bridge event identifiers exchanged between firmware and ESPHome.
enum class EventName : uint8_t {
  kUnknown = 0,
  kPresetPressed,
  kPresetReleased,
  kEncoderTurned,
  kEncoderPressed,
  kBrightnessChanged,
  kAnnouncementRequested,
  kAnnouncementCompleted,
  kModeChanged,
  kVolumeChanged,
};

// Serialized representation of an event frame.
struct EventMessage {
  EventName name{EventName::kUnknown};
  uint32_t timestamp{0};
  int32_t i1{0};
  int32_t i2{0};
  std::string text;
};

const char* eventNameToString(EventName name);
EventName eventNameFromString(const std::string& name);

std::string encodeEvent(const EventMessage& message);
bool decodeEvent(const std::string& frame, EventMessage* out_message);

// High-level bridge command identifiers (ESPHome -> firmware).
enum class CommandName : uint8_t {
  kUnknown = 0,
  kSetMode,
  kSetVolume,
  kSetBrightness,
  kSetMetadata,
  kRequestStatus,
};

// Serialized representation of a command frame.
struct CommandMessage {
  CommandName name{CommandName::kUnknown};
  int32_t mode{-1};
  int32_t preset{-1};
  int32_t value{-1};
  std::string mode_name;
  std::string text;
};

const char* commandNameToString(CommandName name);
CommandName commandNameFromString(const std::string& name);

namespace detail {

inline std::string escapeJson(const std::string& input) {
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
          // Control characters are not expected; skip them.
        } else {
          escaped += c;
        }
        break;
    }
  }
  return escaped;
}

inline bool extractString(const std::string& json, const char* key, std::string* out) {
  const char* key_pos = std::strstr(json.c_str(), key);
  if (!key_pos) {
    return false;
  }
  const char* colon = std::strchr(key_pos + std::strlen(key), ':');
  if (!colon) {
    return false;
  }
  const char* quote_start = std::strchr(colon, '"');
  if (!quote_start) {
    return false;
  }
  ++quote_start;  // move past opening quote
  std::string value;
  value.reserve(32);
  bool escape = false;
  for (const char* p = quote_start; *p; ++p) {
    char c = *p;
    if (escape) {
      switch (c) {
        case 'n': value.push_back('\n'); break;
        case 'r': value.push_back('\r'); break;
        case 't': value.push_back('\t'); break;
        case '"': value.push_back('"'); break;
        case '\\': value.push_back('\\'); break;
        default: value.push_back(c); break;
      }
      escape = false;
      continue;
    }

    if (c == '\\') {
      escape = true;
      continue;
    }

    if (c == '"') {
      if (out) {
        *out = value;
      }
      return true;
    }
    value.push_back(c);
  }
  return false;
}

inline int32_t extractInt(const std::string& json, const char* key, int32_t fallback) {
  const char* key_pos = std::strstr(json.c_str(), key);
  if (!key_pos) {
    return fallback;
  }
  const char* colon = std::strchr(key_pos + std::strlen(key), ':');
  if (!colon) {
    return fallback;
  }
  ++colon;  // move to first digit or sign
  char* end = nullptr;
  const long value = std::strtol(colon, &end, 10);
  if (colon == end) {
    return fallback;
  }
  return static_cast<int32_t>(value);
}

inline std::string eventPrefix(EventName name) {
  std::string payload = "{\"type\":\"";
  payload += eventNameToString(name);
  payload += "\"";
  return payload;
}

inline std::string commandPrefix(CommandName name) {
  std::string payload = "{\"cmd\":\"";
  payload += commandNameToString(name);
  payload += "\"";
  return payload;
}

inline const char* typeKey() { return "\"type\""; }
inline const char* timestampKey() { return "\"ts\""; }
inline const char* i1Key() { return "\"i1\""; }
inline const char* i2Key() { return "\"i2\""; }
inline const char* textKey() { return "\"text\""; }
inline const char* commandKey() { return "\"cmd\""; }
inline const char* modeKey() { return "\"mode\""; }
inline const char* presetKey() { return "\"preset\""; }
inline const char* valueKey() { return "\"value\""; }
inline const char* modeNameKey() { return "\"mode_name\""; }

}  // namespace detail

inline const char* commandNameToString(CommandName name) {
  switch (name) {
    case CommandName::kSetMode: return "set_mode";
    case CommandName::kSetVolume: return "set_volume";
    case CommandName::kSetBrightness: return "set_brightness";
    case CommandName::kSetMetadata: return "set_metadata";
    case CommandName::kRequestStatus: return "request_status";
    case CommandName::kUnknown:
    default:
      return "unknown";
  }
}

inline CommandName commandNameFromString(const std::string& name) {
  if (name == "set_mode") return CommandName::kSetMode;
  if (name == "set_volume") return CommandName::kSetVolume;
  if (name == "set_brightness") return CommandName::kSetBrightness;
  if (name == "set_metadata") return CommandName::kSetMetadata;
  if (name == "request_status") return CommandName::kRequestStatus;
  return CommandName::kUnknown;
}

inline std::string encodeCommand(const CommandMessage& command) {
  std::string json = detail::commandPrefix(command.name);
  if (command.mode >= 0) {
    json += ",\"mode\":" + std::to_string(command.mode);
  }
  if (command.preset >= 0) {
    json += ",\"preset\":" + std::to_string(command.preset);
  }
  if (command.value >= 0) {
    json += ",\"value\":" + std::to_string(command.value);
  }
  if (!command.mode_name.empty()) {
    json += ",\"mode_name\":\"" + detail::escapeJson(command.mode_name) + "\"";
  }
  if (!command.text.empty()) {
    json += ",\"text\":\"" + detail::escapeJson(command.text) + "\"";
  }
  json += "}";
  json.push_back('\n');
  return json;
}

inline bool decodeCommand(const std::string& frame, CommandMessage* out_command) {
  if (!out_command) {
    return false;
  }

  std::string command_name;
  if (!detail::extractString(frame, detail::commandKey(), &command_name)) {
    return false;
  }

  CommandMessage message;
  message.name = commandNameFromString(command_name);
  if (message.name == CommandName::kUnknown) {
    return false;
  }

  message.mode = detail::extractInt(frame, detail::modeKey(), -1);
  message.preset = detail::extractInt(frame, detail::presetKey(), -1);
  message.value = detail::extractInt(frame, detail::valueKey(), -1);

  std::string mode_name;
  if (detail::extractString(frame, detail::modeNameKey(), &mode_name)) {
    message.mode_name = std::move(mode_name);
  }

  std::string text;
  if (detail::extractString(frame, detail::textKey(), &text)) {
    message.text = std::move(text);
  }

  *out_command = std::move(message);
  return true;
}

inline const char* eventNameToString(EventName name) {
  switch (name) {
    case EventName::kPresetPressed: return "preset_pressed";
    case EventName::kPresetReleased: return "preset_released";
    case EventName::kEncoderTurned: return "encoder_turned";
    case EventName::kEncoderPressed: return "encoder_pressed";
    case EventName::kBrightnessChanged: return "brightness_changed";
    case EventName::kAnnouncementRequested: return "announcement_requested";
    case EventName::kAnnouncementCompleted: return "announcement_completed";
    case EventName::kModeChanged: return "mode_changed";
    case EventName::kVolumeChanged: return "volume_changed";
    case EventName::kUnknown:
    default:
      return "unknown";
  }
}

inline EventName eventNameFromString(const std::string& name) {
  if (name == "preset_pressed") return EventName::kPresetPressed;
  if (name == "preset_released") return EventName::kPresetReleased;
  if (name == "encoder_turned") return EventName::kEncoderTurned;
  if (name == "encoder_pressed") return EventName::kEncoderPressed;
  if (name == "brightness_changed") return EventName::kBrightnessChanged;
  if (name == "announcement_requested") return EventName::kAnnouncementRequested;
  if (name == "announcement_completed") return EventName::kAnnouncementCompleted;
  if (name == "mode_changed") return EventName::kModeChanged;
  if (name == "volume_changed") return EventName::kVolumeChanged;
  return EventName::kUnknown;
}

inline std::string encodeEvent(const EventMessage& message) {
  std::string json = detail::eventPrefix(message.name);
  json += ",\"ts\":" + std::to_string(message.timestamp);
  json += ",\"i1\":" + std::to_string(message.i1);
  json += ",\"i2\":" + std::to_string(message.i2);
  if (!message.text.empty()) {
    json += ",\"text\":\"" + detail::escapeJson(message.text) + "\"";
  }
  json += "}";
  json.push_back('\n');
  return json;
}

inline bool decodeEvent(const std::string& frame, EventMessage* out_message) {
  if (!out_message) {
    return false;
  }

  EventMessage message;
  std::string type_string;
  if (!detail::extractString(frame, detail::typeKey(), &type_string)) {
    return false;
  }

  message.name = eventNameFromString(type_string);
  if (message.name == EventName::kUnknown) {
    return false;
  }

  message.timestamp = detail::extractInt(frame, detail::timestampKey(), 0);
  message.i1 = detail::extractInt(frame, detail::i1Key(), 0);
  message.i2 = detail::extractInt(frame, detail::i2Key(), 0);

  std::string text_value;
  if (detail::extractString(frame, detail::textKey(), &text_value)) {
    message.text = std::move(text_value);
  }

  *out_message = std::move(message);
  return true;
}

}  // namespace bridge


