#include "radio_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace radio_controller {

static const char *const TAG = "radio_controller";

void RadioControllerComponent::setup() {
  ESP_LOGI(TAG, "Radio Controller component setup");
  
  // Request initial status from radio firmware
  send_status_request();
}

void RadioControllerComponent::loop() {
  // Read incoming data from radio firmware
  while (available()) {
    char c = read();
    if (c == '\n') {
      if (!rx_buffer_.empty()) {
        ESP_LOGV(TAG, "Received frame: %s", rx_buffer_.c_str());
        process_incoming_frame(rx_buffer_);
        rx_buffer_.clear();
      }
    } else {
      rx_buffer_.push_back(c);
    }
  }
}

void RadioControllerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Radio Controller");
  ESP_LOGCONFIG(TAG, "  Current Mode: %d (%s)", current_mode_, mode_name_.c_str());
  ESP_LOGCONFIG(TAG, "  Volume: %d", volume_);
  ESP_LOGCONFIG(TAG, "  Brightness: %d", brightness_);
  ESP_LOGCONFIG(TAG, "  Entities registered: mode=%s volume=%s brightness=%s metadata=%s",
                mode_select_ ? "yes" : "no",
                volume_number_ ? "yes" : "no", 
                brightness_number_ ? "yes" : "no",
                metadata_sensor_ ? "yes" : "no");
}

void RadioControllerComponent::process_incoming_frame(const std::string& frame) {
  // Simple parsing - look for key patterns in the JSON-like output
  ESP_LOGD(TAG, "Processing frame: %s", frame.c_str());
  
  if (frame.find("\"type_name\":\"settings.brightness\"") != std::string::npos) {
    // Extract brightness value
    size_t pos = frame.find("\"value\":");
    if (pos != std::string::npos) {
      pos += 8; // Skip "value":
      size_t end = frame.find_first_of(",}", pos);
      if (end != std::string::npos) {
        std::string value_str = frame.substr(pos, end - pos);
        int new_brightness = std::stoi(value_str);
        update_brightness(new_brightness);
      }
    }
  }
  else if (frame.find("\"type_name\":\"settings.volume\"") != std::string::npos) {
    // Extract volume value
    size_t pos = frame.find("\"value\":");
    if (pos != std::string::npos) {
      pos += 8; // Skip "value":
      size_t end = frame.find_first_of(",}", pos);
      if (end != std::string::npos) {
        std::string value_str = frame.substr(pos, end - pos);
        int new_volume = std::stoi(value_str);
        update_volume(new_volume);
      }
    }
  }
  else if (frame.find("\"type_name\":\"system.mode\"") != std::string::npos) {
    // Extract mode value
    int new_mode = current_mode_;
    std::string new_mode_name = mode_name_;
    
    size_t pos = frame.find("\"value\":");
    if (pos != std::string::npos) {
      pos += 8; // Skip "value":
      size_t end = frame.find_first_of(",}", pos);
      if (end != std::string::npos) {
        std::string value_str = frame.substr(pos, end - pos);
        new_mode = std::stoi(value_str);
      }
    }
    
    // Extract mode name if present
    pos = frame.find("\"name\":\"");
    if (pos != std::string::npos) {
      pos += 8; // Skip "name":"
      size_t end = frame.find("\"", pos);
      if (end != std::string::npos) {
        new_mode_name = frame.substr(pos, end - pos);
      }
    }
    
    update_mode(new_mode, new_mode_name);
  }
  else if (frame.find("\"type_name\":\"display.message\"") != std::string::npos ||
           frame.find("\"type_name\":\"playback.metadata\"") != std::string::npos) {
    // Extract text value for metadata
    size_t pos = frame.find("\"text\":\"");
    if (pos != std::string::npos) {
      pos += 8; // Skip "text":"
      size_t end = frame.find("\"", pos);
      if (end != std::string::npos) {
        std::string new_metadata = frame.substr(pos, end - pos);
        update_metadata(new_metadata);
      }
    }
  }
}

void RadioControllerComponent::update_mode(int mode, const std::string& mode_name) {
  if (mode != current_mode_ || mode_name != mode_name_) {
    ESP_LOGD(TAG, "Mode changed: %d -> %d (%s -> %s)", current_mode_, mode, mode_name_.c_str(), mode_name.c_str());
    
    current_mode_ = mode;
    mode_name_ = mode_name;
    
    // Update ESPHome entities automatically
    if (mode_select_ && !mode_name.empty()) {
      mode_select_->publish_state(mode_name);
    }
    
    if (current_mode_sensor_) {
      current_mode_sensor_->publish_state(mode_name);
    }
  }
}

void RadioControllerComponent::update_volume(int volume) {
  if (volume != volume_) {
    ESP_LOGD(TAG, "Volume changed: %d -> %d", volume_, volume);
    
    volume_ = volume;
    
    // Update ESPHome entities automatically
    if (volume_number_) {
      volume_number_->publish_state(volume);
    }
    
    if (current_volume_sensor_) {
      current_volume_sensor_->publish_state(volume);
    }
  }
}

void RadioControllerComponent::update_brightness(int brightness) {
  if (brightness != brightness_) {
    ESP_LOGD(TAG, "Brightness changed: %d -> %d", brightness_, brightness);
    
    brightness_ = brightness;
    
    // Update ESPHome entities automatically
    if (brightness_number_) {
      brightness_number_->publish_state(brightness);
    }
    
    if (current_brightness_sensor_) {
      current_brightness_sensor_->publish_state(brightness);
    }
  }
}

void RadioControllerComponent::update_metadata(const std::string& metadata) {
  if (metadata != metadata_) {
    ESP_LOGD(TAG, "Metadata changed: '%s' -> '%s'", metadata_.c_str(), metadata.c_str());
    
    metadata_ = metadata;
    
    // Update ESPHome entities automatically
    if (metadata_sensor_) {
      metadata_sensor_->publish_state(metadata);
    }
  }
}

void RadioControllerComponent::send_mode_command(int mode, const std::string& mode_name, int preset) {
  std::string cmd = "{\"cmd\":\"set_mode\",\"mode\":" + std::to_string(mode);
  if (!mode_name.empty()) {
    cmd += ",\"mode_name\":\"" + mode_name + "\"";
  }
  if (preset >= 0) {
    cmd += ",\"preset\":" + std::to_string(preset);
  }
  cmd += "}";
  send_simple_command(cmd);
}

void RadioControllerComponent::send_volume_command(int volume) {
  std::string cmd = "{\"cmd\":\"set_volume\",\"value\":" + std::to_string(volume) + "}";
  send_simple_command(cmd);
}

void RadioControllerComponent::send_brightness_command(int brightness) {
  std::string cmd = "{\"cmd\":\"set_brightness\",\"value\":" + std::to_string(brightness) + "}";
  send_simple_command(cmd);
}

void RadioControllerComponent::send_metadata_command(const std::string& text) {
  std::string cmd = "{\"cmd\":\"set_metadata\",\"text\":\"" + text + "\"}";
  send_simple_command(cmd);
}

void RadioControllerComponent::send_status_request() {
  std::string cmd = "{\"cmd\":\"request_status\"}";
  send_simple_command(cmd);
}

void RadioControllerComponent::send_simple_command(const std::string& command) {
  ESP_LOGD(TAG, "Sending command: %s", command.c_str());
  write_str(command.c_str());
  write_str("\n");
}

}  // namespace radio_controller
}  // namespace esphome