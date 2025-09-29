#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace radio_controller {

/**
 * ESPHome component that integrates the radio firmware as an external component.
 * This bridges between the radio's internal event system and Home Assistant via ESPHome.
 * 
 * When used in ESPHome builds:
 * - Receives events from radio firmware via UART/API
 * - Exposes HA entities (sensors, switches, etc.)
 * - Sends commands back to radio firmware
 * 
 * The radio firmware runs the same components (RadioHardware, PresetManager, etc.)
 * but uses SerialHomeAssistantBridge instead of StubHomeAssistantBridge.
 */
class HomeAssistantBridgeComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_mode(uint8_t mode);
  void set_mode_from_name(const std::string& mode);
  void set_mode_with_preset(const std::string& mode, uint8_t preset);
  void set_volume(uint8_t volume);
  void set_metadata(const std::string& text);

  void set_uart_parent(uart::UARTComponent *parent) { this->set_parent(parent); }
  void register_frame_callback(std::function<void(const std::string&)> cb);

  uint8_t current_mode() const { return current_mode_; }
  uint8_t volume() const { return volume_; }
  const std::string& metadata() const { return metadata_; }

 private:
  uint8_t current_mode_{0};
  uint8_t volume_{128};
  std::string metadata_;
  std::string rx_buffer_;
  std::function<void(const std::string&)> on_frame_;
};

}  // namespace radio_controller
}  // namespace esphome


