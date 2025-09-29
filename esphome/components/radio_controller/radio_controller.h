#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace radio_controller {

/**
 * ESPHome component that integrates the radio firmware via UART.
 * 
 * This component handles all state synchronization automatically,
 * eliminating the need for complex lambdas in YAML configuration.
 */
class RadioControllerComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Entity registration - called from YAML
  void set_mode_select(select::Select *select) { mode_select_ = select; }
  void set_volume_number(number::Number *number) { volume_number_ = number; }
  void set_brightness_number(number::Number *number) { brightness_number_ = number; }
  void set_metadata_text_sensor(text_sensor::TextSensor *sensor) { metadata_sensor_ = sensor; }
  void set_current_mode_text_sensor(text_sensor::TextSensor *sensor) { current_mode_sensor_ = sensor; }
  void set_current_volume_sensor(sensor::Sensor *sensor) { current_volume_sensor_ = sensor; }
  void set_current_brightness_sensor(sensor::Sensor *sensor) { current_brightness_sensor_ = sensor; }

  // Command methods - called from entity callbacks
  void send_mode_command(int mode, const std::string& mode_name = "", int preset = -1);
  void send_volume_command(int volume);
  void send_brightness_command(int brightness);
  void send_metadata_command(const std::string& text);
  void send_status_request();

  // State accessors for entities that need direct access
  int get_current_mode() const { return current_mode_; }
  std::string get_mode_name() const { return mode_name_; }
  int get_volume() const { return volume_; }
  int get_brightness() const { return brightness_; }
  std::string get_metadata() const { return metadata_; }

  // Set UART parent
  void set_uart_parent(uart::UARTComponent *parent) { this->parent_ = parent; }

 protected:
  void process_incoming_frame(const std::string& frame);
  void send_simple_command(const std::string& command);
  
  // State update methods - automatically sync with entities
  void update_mode(int mode, const std::string& mode_name);
  void update_volume(int volume);
  void update_brightness(int brightness);
  void update_metadata(const std::string& metadata);

  std::string rx_buffer_;
  
  // Current state
  int current_mode_{0};
  std::string mode_name_{"radio"};
  int volume_{128};
  int brightness_{128};
  std::string metadata_;

  // Entity references - automatically updated when state changes
  select::Select *mode_select_{nullptr};
  number::Number *volume_number_{nullptr};
  number::Number *brightness_number_{nullptr};
  text_sensor::TextSensor *metadata_sensor_{nullptr};
  text_sensor::TextSensor *current_mode_sensor_{nullptr};
  sensor::Sensor *current_volume_sensor_{nullptr};
  sensor::Sensor *current_brightness_sensor_{nullptr};
};

}  // namespace radio_controller
}  // namespace esphome