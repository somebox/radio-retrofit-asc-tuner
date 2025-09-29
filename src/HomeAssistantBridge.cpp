#include "HomeAssistantBridge.h"

SerialHomeAssistantBridge::SerialHomeAssistantBridge(HardwareSerial& serial, uint32_t baud)
  : serial_(serial), baud_(baud) {
}

void SerialHomeAssistantBridge::begin() {
  serial_.begin(baud_);
}

void SerialHomeAssistantBridge::update() {
  processIncoming();
}

void SerialHomeAssistantBridge::publishEvent(const events::Event& event) {
  serial_.print("{\"type_id\":");
  serial_.print(event.type_id);
  serial_.print(",\"type_name\":\"");
  serial_.print(event.type_name);
  serial_.print("\",\"timestamp\":");
  serial_.print(event.timestamp);
  serial_.print(",\"value\":");
  serial_.print(event.value.c_str());
  serial_.println("}");
}

void SerialHomeAssistantBridge::processIncoming() {
  while (serial_.available()) {
    char c = serial_.read();
    if (c == '\n') {
      handleCommand(rx_buffer_);
      rx_buffer_.clear();
    } else {
      rx_buffer_ += c;
    }
  }
}

static int parseIntField(const String& line, const char* key, int fallback) {
  String pattern = String("\"") + key + "\":";
  int idx = line.indexOf(pattern);
  if (idx == -1) return fallback;
  int start = idx + pattern.length();
  int end = line.indexOf(',', start);
  if (end == -1) end = line.indexOf('}', start);
  if (end == -1) return fallback;
  return line.substring(start, end).toInt();
}

static String parseStringField(const String& line, const char* key) {
  String pattern = String("\"") + key + "\":\"";
  int idx = line.indexOf(pattern);
  if (idx == -1) return "";
  int start = idx + pattern.length();
  int end = line.indexOf('"', start);
  if (end == -1) return "";
  return line.substring(start, end);
}

void SerialHomeAssistantBridge::handleCommand(const String& line) {
  if (!line.startsWith("{")) {
    Serial.printf("HomeAssistantBridge: malformed frame '%s'\n", line.c_str());
    return;
  }

  if (line.indexOf("set_mode") != -1) {
    int mode = parseIntField(line, "mode", -1);
    String name = parseStringField(line, "mode_name");
    int preset = parseIntField(line, "preset", -1);
    if (handler_) {
      handler_->onSetMode(mode, name, preset);
    }
  } else if (line.indexOf("set_volume") != -1) {
    if (handler_) {
      handler_->onSetVolume(parseIntField(line, "value", -1));
    }
  } else if (line.indexOf("set_brightness") != -1) {
    if (handler_) {
      handler_->onSetBrightness(parseIntField(line, "value", -1));
    }
  } else if (line.indexOf("set_metadata") != -1) {
    if (handler_) {
      String text = parseStringField(line, "text");
      handler_->onSetMetadata(text);
    }
  } else if (line.indexOf("request_status") != -1) {
    if (handler_) {
      handler_->onRequestStatus();
    }
  }
}


