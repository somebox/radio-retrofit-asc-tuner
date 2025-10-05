#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/tca8418_keypad/tca8418_keypad.h"
#include "esphome/components/retrotext_display/retrotext_display.h"
#include "esphome/components/api/custom_api_device.h"
#include <vector>
#include <string>

namespace esphome {

// Forward declarations for optional components
namespace text_sensor {
class TextSensor;
}
namespace select {
class Select;
}

namespace radio_controller {

struct Preset {
  uint8_t row;
  uint8_t column;
  std::string display_text;  // Text to show on display
  std::string target;        // Value to pass to service (e.g., media_id)
  std::string service;       // Service to call (empty = use default)
  std::map<std::string, std::string> data;  // Additional service data
};

class RadioController : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA - 1.0f; }
  
  void set_keypad(tca8418_keypad::TCA8418Component *keypad) { this->keypad_ = keypad; }
  void set_display(retrotext_display::RetroTextDisplay *display) { this->display_ = display; }
  void set_default_service(const std::string &service) { this->default_service_ = service; }
  void set_preset_text_sensor(text_sensor::TextSensor *sensor) { this->preset_text_sensor_ = sensor; }
  void set_preset_target_sensor(text_sensor::TextSensor *sensor) { this->preset_target_sensor_ = sensor; }
  void set_preset_select(select::Select *select) { this->preset_select_ = select; }
  
  void add_preset(uint8_t row, uint8_t column, const std::string &display_text, const std::string &target, const std::string &service);
  void add_preset_data(uint8_t row, uint8_t column, const std::string &key, const std::string &value);
  void set_encoder_button(uint8_t row, uint8_t column);
  
  // Get preset by index for select
  std::vector<std::string> get_preset_names() const;
  void select_preset_by_name(const std::string &name);
  
 protected:
  void handle_key_press_(uint8_t row, uint8_t column);
  void handle_key_release_(uint8_t row, uint8_t column);
  Preset* find_preset_(uint8_t row, uint8_t column);
  Preset* find_preset_by_name_(const std::string &name);
  void activate_preset_(Preset *preset);
  void call_home_assistant_service_(const std::string &service, const std::map<std::string, std::string> &data);
  
  tca8418_keypad::TCA8418Component *keypad_{nullptr};
  retrotext_display::RetroTextDisplay *display_{nullptr};
  text_sensor::TextSensor *preset_text_sensor_{nullptr};
  text_sensor::TextSensor *preset_target_sensor_{nullptr};
  select::Select *preset_select_{nullptr};
  
  std::string default_service_;
  std::vector<Preset> presets_;
  std::string current_preset_name_;
  
  // Optional controls
  bool has_encoder_button_{false};
  uint8_t encoder_row_{0};
  uint8_t encoder_column_{0};
};

}  // namespace radio_controller
}  // namespace esphome
