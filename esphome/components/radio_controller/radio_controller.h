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

// Forward declarations from other components
namespace retrotext_display {
class RetroTextDisplay;
class IS31FL3737Driver;
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
  void set_i2c_bus(i2c::I2CBus *bus) { this->i2c_bus_ = bus; }
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
  
  // Sync LED state on boot
  void sync_preset_led_from_name(const std::string &preset_name);
  void sync_preset_led_from_target(const std::string &target);
  
  // Playlist browsing
  void load_playlist_data(const std::string &json_data);
  void enter_playlist_mode();
  void exit_playlist_mode();
  void scroll_playlist(int direction);  // +1 next, -1 previous
  void select_current_playlist();
  bool is_playlist_playing() const { return this->playlist_playing_; }
  
  // Set text sensor references for mode and index
  void set_radio_mode_sensor(text_sensor::TextSensor *sensor) { this->radio_mode_sensor_ = sensor; }
  
 protected:
  void handle_key_press_(uint8_t row, uint8_t column);
  void handle_key_release_(uint8_t row, uint8_t column);
  void process_encoder_rotation_();
  Preset* find_preset_(uint8_t row, uint8_t column);
  Preset* find_preset_by_name_(const std::string &name);
  void activate_preset_(Preset *preset);
  void call_home_assistant_service_(const std::string &service, const std::map<std::string, std::string> &data);
  
  // Panel LED helpers
  bool init_panel_leds_();
  void update_preset_led_(uint8_t preset_index);
  void update_mode_led_(bool playing);
  void set_vu_meter_target_brightness(uint8_t target);
  void update_vu_meter_slew_();
  
  tca8418_keypad::TCA8418Component *keypad_{nullptr};
  retrotext_display::RetroTextDisplay *display_{nullptr};
  i2c::I2CBus *i2c_bus_{nullptr};
  text_sensor::TextSensor *preset_text_sensor_{nullptr};
  text_sensor::TextSensor *preset_target_sensor_{nullptr};
  text_sensor::TextSensor *radio_mode_sensor_{nullptr};
  select::Select *preset_select_{nullptr};
  
  std::string default_service_;
  std::vector<Preset> presets_;
  std::string current_preset_name_;
  uint8_t current_preset_index_{255};  // 255 = none
  
  // Playlist browsing state
  struct PlaylistItem {
    std::string name;
    std::string uri;
  };
  bool playlist_mode_active_{false};
  std::vector<PlaylistItem> playlists_;
  size_t playlist_index_{0};
  
  // Panel LEDs (internal, automatic)
  std::unique_ptr<esphome::retrotext_display::IS31FL3737Driver> led_driver_;
  bool panel_leds_initialized_{false};
  
  // VU meter backlight slew
  uint8_t vu_meter_current_brightness_{0};
  uint8_t vu_meter_target_brightness_{0};
  uint32_t last_vu_meter_update_{0};
  
  // Optional controls
  bool has_encoder_button_{false};
  uint8_t encoder_row_{0};
  uint8_t encoder_column_{0};
  
  // Encoder rotation tracking (simple edge counting)
  bool encoder_a_state_{false};
  bool encoder_b_state_{false};
  int8_t encoder_last_encoded_{0};
  int32_t encoder_count_{0};         // Running count (like ESP32Encoder getCount())
  int32_t last_encoder_count_{0};    // Last processed count
  
  // Playlist mode state
  bool playlist_playing_{false};  // Currently playing a selected playlist
  uint32_t last_playlist_interaction_{0};  // For timeout
};

}  // namespace radio_controller
}  // namespace esphome
