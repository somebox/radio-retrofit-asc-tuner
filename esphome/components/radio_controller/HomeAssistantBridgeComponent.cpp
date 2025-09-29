#include "HomeAssistantBridgeComponent.h"

#include "esphome/core/log.h"
#include "esphome/components/api/api_server.h"

namespace esphome {
namespace radio_controller {

static const char *const TAG = "homeassistant_bridge";

void HomeAssistantBridgeComponent::setup() {
  ESP_LOGI(TAG, "Home Assistant bridge setup");
}

void HomeAssistantBridgeComponent::loop() {
  while (available()) {
    char c = read();
    if (c == '\n') {
      ESP_LOGD(TAG, "Inbound: %s", rx_buffer_.c_str());
      if (on_frame_) {
        on_frame_(rx_buffer_);
      }
      rx_buffer_.clear();
    } else {
      rx_buffer_.push_back(c);
    }
  }
}

void HomeAssistantBridgeComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Home Assistant Bridge");
  ESP_LOGCONFIG(TAG, "  Current mode: %u", this->current_mode_);
  ESP_LOGCONFIG(TAG, "  Volume: %u", this->volume_);
}

void HomeAssistantBridgeComponent::set_mode(uint8_t mode) {
  this->current_mode_ = mode;
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "{\"cmd\":\"set_mode\",\"mode\":%u}\n", mode);
  write_array((const uint8_t*)buffer, strlen(buffer));
}

void HomeAssistantBridgeComponent::set_mode_from_name(const std::string& mode) {
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "{\"cmd\":\"set_mode\",\"mode_name\":\"%s\"}\n", mode.c_str());
  write_array((const uint8_t*)buffer, strlen(buffer));
}

void HomeAssistantBridgeComponent::set_mode_with_preset(const std::string& mode, uint8_t preset) {
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "{\"cmd\":\"set_mode\",\"mode_name\":\"%s\",\"preset\":%u}\n", mode.c_str(), preset);
  write_array((const uint8_t*)buffer, strlen(buffer));
}

void HomeAssistantBridgeComponent::set_volume(uint8_t volume) {
  this->volume_ = volume;
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "{\"cmd\":\"set_volume\",\"value\":%u}\n", volume);
  write_array((const uint8_t*)buffer, strlen(buffer));
}

void HomeAssistantBridgeComponent::set_metadata(const std::string& text) {
  this->metadata_ = text;
  ESP_LOGD(TAG, "Metadata set: %s", text.c_str());
}

void HomeAssistantBridgeComponent::register_frame_callback(std::function<void(const std::string&)> cb) {
  on_frame_ = std::move(cb);
}

}  // namespace radio_controller
}  // namespace esphome



