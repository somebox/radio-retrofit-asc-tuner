#include "radio_controller.h"

#include "esphome/core/log.h"

namespace esphome {
namespace radio_controller {

static const char *const TAG = "radio_controller";

void RadioController::setup() {
  ESP_LOGI(TAG, "Radio controller setup");
}

void RadioController::loop() {
  // Placeholder for polling bridge state
}

void RadioController::dump_config() {
  ESP_LOGCONFIG(TAG, "Radio Controller");
  ESP_LOGCONFIG(TAG, "  Current mode: %u", this->current_mode_);
  ESP_LOGCONFIG(TAG, "  Volume: %u", this->volume_);
}

void RadioController::set_mode(uint8_t mode) {
  this->current_mode_ = mode;
  ESP_LOGI(TAG, "Set mode to %u", mode);
  // TODO: bridge to firmware via I2C/UART/event bus
}

void RadioController::set_volume(uint8_t volume) {
  this->volume_ = volume;
  ESP_LOGI(TAG, "Set volume to %u", volume);
}

}  // namespace radio_controller
}  // namespace esphome


