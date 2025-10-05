#include "radio_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/retrotext_display/is31fl3737_driver.h"

namespace esphome {
namespace radio_controller {

static const char *const TAG = "radio_controller";

void RadioController::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Radio Controller...");
  
  if (this->keypad_ == nullptr) {
    ESP_LOGE(TAG, "Keypad not configured!");
    this->mark_failed();
    return;
  }
  
  if (this->display_ == nullptr) {
    ESP_LOGE(TAG, "Display not configured!");
    this->mark_failed();
    return;
  }
  
  // Initialize panel LEDs automatically
  if (this->i2c_bus_ != nullptr) {
    this->panel_leds_initialized_ = this->init_panel_leds_();
    if (this->panel_leds_initialized_) {
      ESP_LOGCONFIG(TAG, "Panel LEDs initialized at 0x55");
      
      // Set VU meter target to 10% on boot (will fade in over ~2 seconds)
      this->set_vu_meter_target_brightness(26);  // 10% of 255
    } else {
      ESP_LOGW(TAG, "Panel LEDs not available (optional)");
    }
  }
  
  // Register callbacks with keypad
  this->keypad_->add_on_key_press_callback([this](uint8_t row, uint8_t col, uint8_t key) {
    this->handle_key_press_(row, col);
  });
  
  this->keypad_->add_on_key_release_callback([this](uint8_t row, uint8_t col, uint8_t key) {
    this->handle_key_release_(row, col);
  });
  
  ESP_LOGCONFIG(TAG, "Radio Controller initialized with %d presets", this->presets_.size());
}

void RadioController::loop() {
  // Update VU meter backlight slew
  this->update_vu_meter_slew_();
}

void RadioController::dump_config() {
  ESP_LOGCONFIG(TAG, "Radio Controller:");
  ESP_LOGCONFIG(TAG, "  Default Service: %s", this->default_service_.c_str());
  ESP_LOGCONFIG(TAG, "  Presets: %d", this->presets_.size());
  for (size_t i = 0; i < this->presets_.size(); i++) {
    const auto &preset = this->presets_[i];
    const char *service = preset.service.empty() ? this->default_service_.c_str() : preset.service.c_str();
    ESP_LOGCONFIG(TAG, "    [%d] Row=%d, Col=%d, Display='%s', Target='%s', Service='%s'",
                  i + 1, preset.row, preset.column, preset.display_text.c_str(), 
                  preset.target.c_str(), service);
    if (!preset.data.empty()) {
      for (const auto &kv : preset.data) {
        ESP_LOGCONFIG(TAG, "        Data: %s=%s", kv.first.c_str(), kv.second.c_str());
      }
    }
  }
  if (this->has_encoder_button_) {
    ESP_LOGCONFIG(TAG, "  Encoder Button: Row=%d, Col=%d", this->encoder_row_, this->encoder_column_);
  }
}

void RadioController::add_preset(uint8_t row, uint8_t column, const std::string &display_text, 
                                  const std::string &target, const std::string &service) {
  Preset preset;
  preset.row = row;
  preset.column = column;
  preset.display_text = display_text;
  preset.target = target;
  preset.service = service;
  this->presets_.push_back(preset);
  
  ESP_LOGD(TAG, "Added preset: Row=%d, Col=%d, Display='%s', Target='%s'", 
           row, column, display_text.c_str(), target.c_str());
}

void RadioController::add_preset_data(uint8_t row, uint8_t column, const std::string &key, const std::string &value) {
  // Find the preset and add data
  Preset *preset = this->find_preset_(row, column);
  if (preset != nullptr) {
    preset->data[key] = value;
    ESP_LOGD(TAG, "Added data to preset [%d,%d]: %s=%s", row, column, key.c_str(), value.c_str());
  } else {
    ESP_LOGW(TAG, "Cannot add data: preset [%d,%d] not found", row, column);
  }
}

void RadioController::set_encoder_button(uint8_t row, uint8_t column) {
  this->has_encoder_button_ = true;
  this->encoder_row_ = row;
  this->encoder_column_ = column;
  ESP_LOGD(TAG, "Set encoder button: Row=%d, Col=%d", row, column);
}

void RadioController::handle_key_press_(uint8_t row, uint8_t column) {
  ESP_LOGD(TAG, "Key pressed: row=%d, col=%d", row, column);
  
  // Check if this is a preset button
  Preset *preset = this->find_preset_(row, column);
  if (preset != nullptr) {
    this->activate_preset_(preset);
    return;
  }
  
  // Check if this is the encoder button
  if (this->has_encoder_button_ && row == this->encoder_row_ && column == this->encoder_column_) {
    ESP_LOGI(TAG, "Encoder button pressed - stopping playback");
    
    // Update display
    if (this->display_ != nullptr) {
      this->display_->set_text("STOPPED");
    }
    
    // Clear current preset indicators
    if (this->preset_text_sensor_ != nullptr) {
      this->preset_text_sensor_->publish_state("Stopped");
    }
    if (this->preset_target_sensor_ != nullptr) {
      this->preset_target_sensor_->publish_state("");
    }
    if (this->preset_select_ != nullptr) {
      this->preset_select_->publish_state("");
    }
    
    // Update LEDs (turn off mode LED, clear preset LED)
    this->update_mode_led_(false);
    if (this->panel_leds_initialized_ && this->led_driver_) {
      this->led_driver_->clear();
      this->led_driver_->show();
    }
    this->current_preset_index_ = 255;
    
    // Fade VU meter back to 10% when stopped
    this->set_vu_meter_target_brightness(26);  // 10% of 255
    
    // Stop media playback
    std::map<std::string, std::string> service_data;
    service_data["entity_id"] = "media_player.macstudio_local";
    this->call_home_assistant_service_("media_player.media_stop", service_data);
    
    return;
  }
  
  // Unknown button
  ESP_LOGD(TAG, "Unhandled key press: row=%d, col=%d", row, column);
}

void RadioController::handle_key_release_(uint8_t row, uint8_t column) {
  // Currently not handling releases, but could add feedback here
  ESP_LOGV(TAG, "Key released: row=%d, col=%d", row, column);
}

Preset* RadioController::find_preset_(uint8_t row, uint8_t column) {
  for (auto &preset : this->presets_) {
    if (preset.row == row && preset.column == column) {
      return &preset;
    }
  }
  return nullptr;
}

Preset* RadioController::find_preset_by_name_(const std::string &name) {
  for (auto &preset : this->presets_) {
    if (preset.display_text == name) {
      return &preset;
    }
  }
  return nullptr;
}

void RadioController::activate_preset_(Preset *preset) {
  if (preset == nullptr) {
    return;
  }
  
  ESP_LOGI(TAG, "Preset activated: '%s' (target: '%s')", 
           preset->display_text.c_str(), preset->target.c_str());
  
  // Store current preset name and find index
  this->current_preset_name_ = preset->display_text;
  uint8_t preset_index = 0;
  for (size_t i = 0; i < this->presets_.size(); i++) {
    if (&this->presets_[i] == preset) {
      preset_index = i;
      this->current_preset_index_ = i;
      break;
    }
  }
  
  // Update display
  if (this->display_ != nullptr) {
    this->display_->set_text(preset->display_text.c_str());
  }
  
  // Update text sensor
  if (this->preset_text_sensor_ != nullptr) {
    this->preset_text_sensor_->publish_state(preset->display_text);
  }
  
  // Update target sensor (for automation to read)
  if (this->preset_target_sensor_ != nullptr) {
    this->preset_target_sensor_->publish_state(preset->target);
    ESP_LOGD(TAG, "Published media_id: '%s'", preset->target.c_str());
  }
  
  // Update select
  if (this->preset_select_ != nullptr) {
    this->preset_select_->publish_state(preset->display_text);
  }
  
  // Update preset LED (automatic, built-in)
  this->update_preset_led_(preset_index);
  
  // Update mode LED to show "playing"
  this->update_mode_led_(true);
  
  // Set VU meter to 80% when playing
  this->set_vu_meter_target_brightness(204);  // 80% of 255
  
  // Determine which service to call
  std::string service = preset->service.empty() ? this->default_service_ : preset->service;
  
  // Build service data
  std::map<std::string, std::string> service_data = preset->data;
  
  // Add target if specified (as 'target' key)
  if (!preset->target.empty()) {
    service_data["target"] = preset->target;
  }
  
  // Call Home Assistant service
  if (!service.empty()) {
    this->call_home_assistant_service_(service, service_data);
  }
}

std::vector<std::string> RadioController::get_preset_names() const {
  std::vector<std::string> names;
  for (const auto &preset : this->presets_) {
    names.push_back(preset.display_text);
  }
  return names;
}

void RadioController::select_preset_by_name(const std::string &name) {
  Preset *preset = this->find_preset_by_name_(name);
  if (preset != nullptr) {
    this->activate_preset_(preset);
  } else {
    ESP_LOGW(TAG, "Preset not found: '%s'", name.c_str());
  }
}

void RadioController::sync_preset_led_from_name(const std::string &preset_name) {
  if (preset_name.empty() || preset_name == "Stopped" || preset_name == "Ready") {
    // No active preset, clear LEDs
    ESP_LOGD(TAG, "Syncing LEDs: No active preset");
    return;
  }
  
  // Find the preset by name
  for (size_t i = 0; i < this->presets_.size(); i++) {
    if (this->presets_[i].display_text == preset_name) {
      ESP_LOGI(TAG, "Syncing preset LED by name: '%s' (index %d)", preset_name.c_str(), i);
      this->current_preset_index_ = i;
      this->update_preset_led_(i);
      this->update_mode_led_(true);
      this->set_vu_meter_target_brightness(204);  // 80% when playing
      return;
    }
  }
  
  ESP_LOGW(TAG, "Could not sync LED: preset '%s' not found", preset_name.c_str());
}

void RadioController::sync_preset_led_from_target(const std::string &target) {
  if (target.empty()) {
    ESP_LOGD(TAG, "Syncing LEDs: No target specified");
    return;
  }
  
  // Find the preset by target (media_id)
  for (size_t i = 0; i < this->presets_.size(); i++) {
    if (this->presets_[i].target == target) {
      ESP_LOGI(TAG, "Syncing preset LED by target: '%s' -> '%s' (index %d)", 
               target.c_str(), this->presets_[i].display_text.c_str(), i);
      this->current_preset_index_ = i;
      this->update_preset_led_(i);
      this->update_mode_led_(true);
      this->set_vu_meter_target_brightness(204);  // 80% when playing
      return;
    }
  }
  
  ESP_LOGD(TAG, "Could not sync LED: target '%s' not found in presets", target.c_str());
}

void RadioController::call_home_assistant_service_(const std::string &service, 
                                                     const std::map<std::string, std::string> &data) {
  ESP_LOGD(TAG, "Calling Home Assistant service: %s", service.c_str());
  
#ifdef USE_API
  // Use ESPHome's API to call Home Assistant service
  // Create the service call message
  api::HomeassistantServiceResponse call;
  
  // Check if this is a script call - use script.turn_on format
  if (service.find("script.") == 0) {
    // Calling a script - use script.turn_on with entity_id
    call.service = "script.turn_on";
    call.is_event = false;
    
    // Add entity_id for the script
    api::HomeassistantServiceMap entity_entry;
    entity_entry.key = "entity_id";
    entity_entry.value = service;  // e.g., "script.radio_play_preset"
    call.data.push_back(entity_entry);
    ESP_LOGD(TAG, "  entity_id=%s", service.c_str());
    
    // Add script parameters
    for (const auto &kv : data) {
      api::HomeassistantServiceMap entry;
      entry.key = kv.first;
      entry.value = kv.second;
      call.data.push_back(entry);
      ESP_LOGD(TAG, "  %s=%s", kv.first.c_str(), kv.second.c_str());
    }
  } else {
    // Regular service call
    call.service = service;
    call.is_event = false;
    
    // Add service data
    for (const auto &kv : data) {
      api::HomeassistantServiceMap entry;
      entry.key = kv.first;
      entry.value = kv.second;
      call.data.push_back(entry);
      ESP_LOGD(TAG, "  Data: %s=%s", kv.first.c_str(), kv.second.c_str());
    }
  }
  
  // Send the service call
  api::global_api_server->send_homeassistant_service_call(call);
  ESP_LOGD(TAG, "Service call sent successfully");
#else
  ESP_LOGW(TAG, "API not available, cannot call service: %s", service.c_str());
#endif
}

// Panel LED Management
bool RadioController::init_panel_leds_() {
  // LED positions from HardwareConfig.h
  constexpr uint8_t LED_I2C_ADDRESS = 0x55;
  
  this->led_driver_.reset(new esphome::retrotext_display::IS31FL3737Driver());
  
  if (!this->led_driver_->begin(LED_I2C_ADDRESS, this->i2c_bus_)) {
    ESP_LOGD(TAG, "Panel LEDs not found at 0x55 (optional hardware)");
    this->led_driver_.reset();
    return false;
  }
  
  // Set reasonable brightness for panel LEDs
  this->led_driver_->set_global_current(128);
  
  // Clear all LEDs
  this->led_driver_->clear();
  this->led_driver_->show();
  
  return true;
}

void RadioController::update_preset_led_(uint8_t preset_index) {
  if (!this->panel_leds_initialized_ || !this->led_driver_) {
    return;
  }
  
  // LED positions (SW, CS) for 8 presets from HardwareConfig.h
  constexpr struct { uint8_t sw; uint8_t cs; } PRESET_LEDS[8] = {
    {3, 3},  // Preset 1
    {3, 2},  // Preset 2
    {3, 1},  // Preset 3
    {3, 0},  // Preset 4
    {3, 8},  // Preset 5
    {3, 7},  // Preset 6
    {3, 6},  // Preset 7
    {3, 5}   // Memory/Preset 8
  };
  
  if (preset_index >= 8) {
    return;
  }
  
  // Turn off all preset LEDs
  for (uint8_t i = 0; i < 8; i++) {
    this->led_driver_->set_pixel(PRESET_LEDS[i].cs, PRESET_LEDS[i].sw, 0);
  }
  
  // Turn on the active preset LED
  this->led_driver_->set_pixel(PRESET_LEDS[preset_index].cs, PRESET_LEDS[preset_index].sw, 255);
  this->led_driver_->show();
  
  ESP_LOGD(TAG, "Updated preset LED: %d", preset_index);
}

void RadioController::update_mode_led_(bool playing) {
  if (!this->panel_leds_initialized_ || !this->led_driver_) {
    return;
  }
  
  // Mode LED positions (SW, CS) from HardwareConfig.h
  // Mode 0 = Stereo (show when playing)
  constexpr uint8_t STEREO_LED_SW = 0;
  constexpr uint8_t STEREO_LED_CS = 7;
  
  uint8_t brightness = playing ? 255 : 0;
  this->led_driver_->set_pixel(STEREO_LED_CS, STEREO_LED_SW, brightness);
  this->led_driver_->show();
  
  ESP_LOGD(TAG, "Updated mode LED: Stereo %s", playing ? "ON" : "OFF");
}

void RadioController::set_vu_meter_target_brightness(uint8_t target) {
  this->vu_meter_target_brightness_ = target;
  ESP_LOGD(TAG, "VU meter target brightness set to: %d", target);
}

void RadioController::update_vu_meter_slew_() {
  if (!this->panel_leds_initialized_ || !this->led_driver_) {
    return;
  }
  
  // Update every 25ms for smooth animation
  uint32_t now = millis();
  if (now - this->last_vu_meter_update_ < 25) {
    return;
  }
  this->last_vu_meter_update_ = now;
  
  // If we're at target, nothing to do
  if (this->vu_meter_current_brightness_ == this->vu_meter_target_brightness_) {
    return;
  }
  
  // Calculate step size (1 unit per update = ~2 seconds for full 0-77 range)
  int diff = this->vu_meter_target_brightness_ - this->vu_meter_current_brightness_;
  if (diff > 0) {
    this->vu_meter_current_brightness_++;
  } else {
    this->vu_meter_current_brightness_--;
  }
  
  // VU meter backlight positions from HardwareConfig.h
  // Row 2: "Tuning Backlight" at CS=9, "Signal Backlight" at CS=10
  constexpr uint8_t VU_METER_ROW = 2;
  constexpr uint8_t TUNING_BACKLIGHT_CS = 9;
  constexpr uint8_t SIGNAL_BACKLIGHT_CS = 10;
  
  // Update both backlight LEDs
  this->led_driver_->set_pixel(TUNING_BACKLIGHT_CS, VU_METER_ROW, this->vu_meter_current_brightness_);
  this->led_driver_->set_pixel(SIGNAL_BACKLIGHT_CS, VU_METER_ROW, this->vu_meter_current_brightness_);
  this->led_driver_->show();
  
  ESP_LOGV(TAG, "VU meter brightness: %d -> %d", 
           this->vu_meter_current_brightness_, this->vu_meter_target_brightness_);
}

}  // namespace radio_controller
}  // namespace esphome
