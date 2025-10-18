#include "radio_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/retrotext_display/is31fl3737_driver.h"
#include <ArduinoJson.h>

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
  
  // Initialize preset storage from flash
  this->load_presets_from_flash_();
  
  // Note: Preset slot sensors removed - caused API connection issues
  // Preset data is accessible via select component and current_preset sensor
  
  // Register callbacks with keypad
  this->keypad_->add_on_key_press_callback([this](uint8_t row, uint8_t col, uint8_t key) {
    this->handle_key_press_(row, col);
  });
  
  this->keypad_->add_on_key_release_callback([this](uint8_t row, uint8_t col, uint8_t key) {
    this->handle_key_release_(row, col);
  });
  
  // Build initial browse list from stored presets
  build_browse_list_();
  
  // Publish preset sensors to HA
  this->publish_preset_sensors_();
  
  ESP_LOGCONFIG(TAG, "Radio Controller initialized with %d presets", this->presets_.size());
}

void RadioController::loop() {
  // Update VU meter backlight slew
  this->update_vu_meter_slew_();
  
  // Check for browse mode timeout (5 seconds of inactivity)
  if (this->browse_mode_active_ && this->last_browse_interaction_ > 0) {
    uint32_t elapsed = millis() - this->last_browse_interaction_;
    if (elapsed > 5000) {  // 5 second timeout
      ESP_LOGI(TAG, "Browse timeout - returning to now-playing");
      this->exit_browse_mode_();
    }
  }
  
  // Auto-dismiss "Preset X: Saved" message after 2 seconds
  if (this->preset_saved_message_time_ > 0) {
    uint32_t elapsed = millis() - this->preset_saved_message_time_;
    if (elapsed > 2000) {  // 2 second delay
      this->preset_saved_message_time_ = 0;  // Clear timer
      
      // Restore now-playing display
      if (this->is_playing_ && !this->browse_mode_active_) {
        if (this->currently_playing_index_ >= 0 && 
            this->currently_playing_index_ < (int)this->browse_items_.size()) {
          // Show station name or metadata
          std::string display_text;
          if (!this->now_playing_metadata_.empty() && 
              this->now_playing_metadata_ != "Ready" && 
              this->now_playing_metadata_ != "Stopped") {
            display_text = this->format_display_text_(this->now_playing_metadata_);
          } else {
            display_text = this->format_display_text_(this->browse_items_[this->currently_playing_index_].name);
          }
          
          if (this->display_) {
            this->display_->set_text(display_text.c_str());
            ESP_LOGD(TAG, "Auto-dismissed 'Preset Saved' message, restored: '%s'", display_text.c_str());
          }
        }
      }
    }
  }
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
  
  // Check if this is an encoder rotation channel (A or B)
  // Actual hardware mapping (may differ from HardwareConfig.h)
  if (this->has_encoder_button_ && row == this->encoder_row_) {
    // Encoder channel A (col 2) or B (col 3)
    if (column == 3) {  // Encoder B
      this->encoder_b_state_ = true;
      this->process_encoder_rotation_();
      return;
    } else if (column == 2) {  // Encoder A
      this->encoder_a_state_ = true;
      this->process_encoder_rotation_();
      return;
    }
  }
  
  // Check if this is the memory button (if configured)
  if (this->has_memory_button_ && row == this->memory_button_row_ && column == this->memory_button_col_) {
    // Record press time for long-press detection
    this->memory_button_press_time_ = millis();
    return;
  }
  
  // Check if this is a preset button
  Preset *preset = this->find_preset_(row, column);
  if (preset != nullptr) {
    // If in save preset mode, save currently playing station to this preset slot
    if (this->save_preset_mode_) {
      ESP_LOGI(TAG, "SAVE MODE: Preset button pressed at row=%d, col=%d", row, column);
      
      // Find preset slot index
      for (size_t i = 0; i < this->presets_.size(); i++) {
        if (this->presets_[i].row == row && this->presets_[i].column == column) {
          // Get currently playing item from browse list (works for presets, playlists, and favorites)
          if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
            const auto &playing_item = browse_items_[currently_playing_index_];
            
            // Use the item's name and target (preserves playlist URI, not track metadata)
            ESP_LOGI(TAG, "SAVE MODE: Saving '%s' (target: %s) to slot %d", 
                     playing_item.name.c_str(), playing_item.target.c_str(), i + 1);
            
            // Save to the selected slot
            this->save_preset_to_slot(i, playing_item.target, playing_item.name);
            
            // DON'T activate the preset - stay on current station
            // Just update the browse list and LED to reflect the save
            this->build_browse_list_();
            this->update_leds_for_browse_();
            
            // After brief confirmation, restore playing display
            // (save_preset_to_slot already shows "PRESET X: SAVED")
            // The display will automatically update on next metadata update
            
            ESP_LOGI(TAG, "SAVE MODE: Complete - staying on current station");
          } else {
            ESP_LOGW(TAG, "SAVE MODE: Error - no valid currently playing item");
            if (this->display_) {
              this->display_->set_text("SAVE FAILED");
            }
          }
          this->save_preset_mode_ = false;
          return;
        }
      }
      // If not a valid preset slot, cancel save mode
      ESP_LOGI(TAG, "SAVE MODE: Cancelled - button not a preset slot");
      this->save_preset_mode_ = false;
      return;
    }
    
    // Regular preset button - play that preset
    this->activate_preset_(preset);
    return;
  }
  
  // Check if this is the encoder button - NEW: play/stop toggle
  if (this->has_encoder_button_ && row == this->encoder_row_ && column == this->encoder_column_) {
    ESP_LOGI(TAG, "Encoder button pressed: toggle play/stop");
    this->toggle_play_stop_();
    return;
  }
  
  // Unknown button
  ESP_LOGD(TAG, "Unhandled key press: row=%d, col=%d", row, column);
}

void RadioController::handle_key_release_(uint8_t row, uint8_t column) {
  // Handle encoder channel releases - update state and process
  // The quadrature decoder tracks detent progress and prevents double-counting
  if (this->has_encoder_button_ && row == this->encoder_row_) {
    if (column == 3) {  // Encoder B
      this->encoder_b_state_ = false;
      this->process_encoder_rotation_();
      return;
    } else if (column == 2) {  // Encoder A
      this->encoder_a_state_ = false;
      this->process_encoder_rotation_();
      return;
    }
  }
  
  // Check if this is memory button release - toggle save preset mode
  if (this->has_memory_button_ && row == this->memory_button_row_ && column == this->memory_button_col_) {
    // Toggle save preset mode
    if (this->save_preset_mode_) {
      // Already in save mode - tap again exits without saving
      ESP_LOGI(TAG, "SAVE MODE: Cancelled by memory button (tap to exit)");
      this->save_preset_mode_ = false;
      
      // Exit browse mode and return to now-playing display
      if (this->browse_mode_active_) {
        this->exit_browse_mode_();
      } else {
        // Restore now-playing display
        if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
          if (this->display_ != nullptr) {
            std::string display_text = this->format_display_text_(browse_items_[currently_playing_index_].name);
            this->display_->set_text(display_text.c_str());
          }
        }
      }
      
      // Update LEDs
      this->update_leds_for_browse_();
    } else {
      // Enter save preset mode
      if (currently_playing_index_ < 0 || !this->is_playing_) {
        ESP_LOGW(TAG, "SAVE MODE: Cannot enter - no station currently playing");
        if (this->display_) {
          this->display_->set_text("NO STATION PLAYING");
        }
      } else {
        const auto &item = browse_items_[currently_playing_index_];
        ESP_LOGI(TAG, "SAVE MODE: Entered - will save '%s' (target: %s)", 
                 item.name.c_str(), item.target.c_str());
        this->save_preset_mode_ = true;
        if (this->display_) {
          this->display_->set_text("SELECT PRESET (TAP MEMORY TO CANCEL)");
        }
      }
    }
    return;
  }
  
  ESP_LOGV(TAG, "Key released: row=%d, col=%d", row, column);
}

void RadioController::process_encoder_rotation_() {
  // Full quadrature decoding with Gray code state machine
  // Tracks 4-step detent cycle for reliable rotation detection
  
  uint8_t current_state = (this->encoder_a_state_ ? 0b10 : 0) | (this->encoder_b_state_ ? 0b01 : 0);
  uint8_t old_state = this->encoder_last_encoded_;
  
  // Only process if state actually changed
  if (current_state == old_state) {
    return;
  }
  
  // Update state for next comparison
  this->encoder_last_encoded_ = current_state;
  
  // Detect valid Gray code transitions
  bool valid_transition = false;
  int8_t direction = 0;
  
  // Clockwise: 00 -> 01 -> 11 -> 10 -> 00
  if ((old_state == 0b00 && current_state == 0b01) ||
      (old_state == 0b01 && current_state == 0b11) ||
      (old_state == 0b11 && current_state == 0b10) ||
      (old_state == 0b10 && current_state == 0b00)) {
    direction = 1;
    valid_transition = true;
  }
  // Counter-clockwise: 00 -> 10 -> 11 -> 01 -> 00
  else if ((old_state == 0b00 && current_state == 0b10) ||
           (old_state == 0b10 && current_state == 0b11) ||
           (old_state == 0b11 && current_state == 0b01) ||
           (old_state == 0b01 && current_state == 0b00)) {
    direction = -1;
    valid_transition = true;
  }
  
  if (!valid_transition) {
    ESP_LOGV(TAG, "Encoder: Invalid transition %d->%d, resetting", old_state, current_state);
    this->encoder_detent_count_ = 0;
    return;
  }
  
  // Track detent progress (2 steps per detent for this encoder hardware)
  this->encoder_detent_count_ += direction;
  
  ESP_LOGV(TAG, "Encoder: %s transition %d->%d, detent: %d/2",
           direction > 0 ? "CW" : "CCW", old_state, current_state, 
           abs(this->encoder_detent_count_));
  
  // Complete detent reached (2 steps, not 4)
  bool detent_complete = false;
  if (this->encoder_detent_count_ >= 2) {
    this->encoder_count_++;
    this->encoder_detent_count_ = 0;
    detent_complete = true;
    ESP_LOGD(TAG, "Encoder: CW detent complete (count:%d)", this->encoder_count_);
  } else if (this->encoder_detent_count_ <= -2) {
    this->encoder_count_--;
    this->encoder_detent_count_ = 0;
    detent_complete = true;
    ESP_LOGD(TAG, "Encoder: CCW detent complete (count:%d)", this->encoder_count_);
  }
  
  // NEW: Handle encoder turns - always scrolls unified browse list
  // Direction inverted: CW = previous, CCW = next (per user request)
  if (detent_complete) {
    int32_t encoder_change = this->encoder_count_ - this->last_encoder_count_;
    
    if (encoder_change > 0) {
      ESP_LOGI(TAG, "Encoder: CW scroll (previous)");
      this->scroll_browse_(-1);  // CW = backward
      this->last_encoder_count_ = this->encoder_count_;
    } else if (encoder_change < 0) {
      ESP_LOGI(TAG, "Encoder: CCW scroll (next)");
      this->scroll_browse_(1);   // CCW = forward
      this->last_encoder_count_ = this->encoder_count_;
    }
  }
  
  // Reset count periodically to prevent overflow
  if (abs(this->encoder_count_) > 1000) {
    this->encoder_count_ = 0;
    this->last_encoder_count_ = 0;
  }
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
  
  // IMPORTANT: Also set currently_playing_index_ in browse_items_
  // This is needed for display updates to show station name with icon
  for (size_t i = 0; i < browse_items_.size(); i++) {
    if (browse_items_[i].type == BrowseItem::PRESET && browse_items_[i].preset_index == preset_index) {
      currently_playing_index_ = i;
      // Note: browse_index_ stays where user was browsing (don't sync with currently_playing)
      ESP_LOGD(TAG, "Set currently_playing_index_ to %d for preset '%s'", i, preset->display_text.c_str());
      break;
    }
  }
  
  // Update display (will show play icon since we're starting playback)
  // Set is_playing_ = true immediately for better UX
  is_playing_ = true;
  
  // Record activation time to delay metadata display (show station name first)
  this->preset_activation_time_ = millis();
  
  if (this->display_ != nullptr) {
    std::string display_text = this->format_display_text_(preset->display_text);
    this->display_->set_text(display_text.c_str());
    ESP_LOGD(TAG, "Display updated: '%s' (will show station name for 3 seconds)", display_text.c_str());
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
  
  // First try: Find in browse_items_ list (includes both presets and favorites)
  for (size_t i = 0; i < browse_items_.size(); i++) {
    const auto &item = browse_items_[i];
    // Check if name contains the preset name (partial match for flexibility)
    if (item.name.find(preset_name) != std::string::npos || 
        preset_name.find(item.name) != std::string::npos) {
      if (item.type == BrowseItem::PRESET && item.preset_index >= 0 && item.preset_index < 7) {
        ESP_LOGI(TAG, "Syncing preset LED by name: '%s' matched browse item '%s' (preset %d)", 
                 preset_name.c_str(), item.name.c_str(), item.preset_index);
        this->current_preset_index_ = item.preset_index;
        this->currently_playing_index_ = i;
        this->is_playing_ = true;
        this->update_preset_led_(item.preset_index);
        this->update_leds_for_browse_();
        this->update_mode_led_(true);
        this->set_vu_meter_target_brightness(204);  // 80% when playing
        return;
      }
    }
  }
  
  ESP_LOGD(TAG, "LED sync: station '%s' not saved in any preset slot", preset_name.c_str());
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
  
  // LED positions (SW, CS) for 7 presets from HardwareConfig.h
  // Memory button at {3, 5} handled separately in update_leds_for_browse_()
  constexpr struct { uint8_t sw; uint8_t cs; } PRESET_LEDS[7] = {
    {3, 3},  // Preset 1
    {3, 2},  // Preset 2
    {3, 1},  // Preset 3
    {3, 0},  // Preset 4
    {3, 8},  // Preset 5
    {3, 7},  // Preset 6
    {3, 6}   // Preset 7
  };
  
  if (preset_index >= 7) {
    return;
  }
  
  // Turn off all preset LEDs
  for (uint8_t i = 0; i < 7; i++) {
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

// Playlist Browsing Methods

void RadioController::load_playlist_data(const std::string &json_data) {
  ESP_LOGI(TAG, "Loading playlist data: %s", json_data.c_str());
  
  // Clear existing playlists
  this->playlists_.clear();
  this->playlist_index_ = 0;
  
  // Parse JSON using ArduinoJson
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json_data);
  
  if (error) {
    ESP_LOGE(TAG, "Failed to parse JSON: %s", error.c_str());
    return;
  }
  
  // Expect array of objects: [{"name": "...", "uri": "..."}, ...]
  if (doc.is<JsonArray>()) {
    JsonArray array = doc.as<JsonArray>();
    
    for (JsonVariant item : array) {
      if (item.is<JsonObject>()) {
        JsonObject obj = item.as<JsonObject>();
        
        // Check if both name and uri fields exist
        if (obj["name"].is<const char*>() && obj["uri"].is<const char*>()) {
          PlaylistItem playlist;
          playlist.name = obj["name"].as<std::string>();
          playlist.uri = obj["uri"].as<std::string>();
          this->playlists_.push_back(playlist);
          
          ESP_LOGD(TAG, "Added playlist: %s", playlist.name.c_str());
        }
      }
    }
  }
  
  ESP_LOGI(TAG, "Loaded %d playlists", this->playlists_.size());
  
  // Rebuild browse list to include new playlists
  build_browse_list_();
}

// ============================================================================
// NEW UNIFIED BROWSE SYSTEM
// ============================================================================

void RadioController::build_browse_list_() {
  browse_items_.clear();
  
  // Add all 7 preset slots (including empty ones)
  for (uint8_t i = 0; i < 7; i++) {
    const StoredPreset &stored = stored_presets_[i];
    
    BrowseItem item;
    item.type = BrowseItem::PRESET;
    
    // Use stored preset if valid, otherwise fall back to YAML preset
    if (stored.is_valid) {
      item.name = stored.display_name;
      item.target = stored.media_id;
    } else if (i < this->presets_.size()) {
      // Use YAML preset when stored preset is invalid/empty
      item.name = this->presets_[i].display_text;
      item.target = this->presets_[i].target;
    } else {
      // No YAML preset either - truly empty
      item.name = stored.display_name;  // "Empty Slot X"
      item.target = "";
    }
    
    item.preset_index = i;
    
    // Find row/column from runtime presets if available
    if (i < this->presets_.size()) {
      item.row = this->presets_[i].row;
      item.column = this->presets_[i].column;
    } else {
      item.row = 0;
      item.column = 0;
    }
    
    browse_items_.push_back(item);
  }
  
  // Add separator if we have additional content
  if (!this->playlists_.empty() || !this->all_favorites_.empty()) {
    BrowseItem separator;
    separator.type = BrowseItem::PRESET;  // Use PRESET type for separators
    separator.name = "--- ALL FAVORITES ---";
    separator.target = "";
    separator.preset_index = -1;
    separator.row = 0;
    separator.column = 0;
    browse_items_.push_back(separator);
  }
  
  // Add all playlists
  for (const auto &playlist : this->playlists_) {
    BrowseItem item;
    item.type = BrowseItem::PLAYLIST;
    item.name = playlist.name;
    item.target = playlist.uri;
    item.preset_index = -1;  // Not a preset
    item.row = 0;
    item.column = 0;
    browse_items_.push_back(item);
  }
  
  // Add all favorites (radios + additional playlists from MA)
  for (const auto &fav : this->all_favorites_) {
    BrowseItem item;
    item.type = BrowseItem::FAVORITE;
    item.name = fav.name;
    item.target = fav.uri;
    item.preset_index = -1;  // Not a preset slot
    item.row = 0;
    item.column = 0;
    browse_items_.push_back(item);
  }
  
  ESP_LOGI(TAG, "Built browse list: %d items (7 preset slots, %d playlists, %d favorites)",
           browse_items_.size(), 
           this->playlists_.size(),
           this->all_favorites_.size());
}

void RadioController::enter_browse_mode_() {
  this->browse_mode_active_ = true;
  this->last_browse_interaction_ = millis();
  
  // Show current selection
  // Show play/stop icon ONLY if this is the currently playing station
  if (!browse_items_.empty() && browse_index_ < browse_items_.size()) {
    if (this->display_ != nullptr) {
      std::string display_text;
      
      // Add icon only if this is the currently playing item
      if ((int)browse_index_ == currently_playing_index_) {
        display_text = this->format_display_text_(browse_items_[browse_index_].name);
      } else {
        display_text = browse_items_[browse_index_].name;
      }
      
      this->display_->set_text(display_text.c_str());
    }
  }
  
  update_leds_for_browse_();
  
  ESP_LOGI(TAG, "Entered browse mode at index %d/%d", browse_index_, browse_items_.size());
}

void RadioController::exit_browse_mode_() {
  this->browse_mode_active_ = false;
  
  // Return to showing now-playing
  if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
    if (this->display_ != nullptr) {
      // If playing with real metadata, show it; otherwise show station name
      // Filter out "Ready" and other non-metadata values
      bool has_real_metadata = !now_playing_metadata_.empty() && 
                               now_playing_metadata_ != "Ready" && 
                               now_playing_metadata_ != "ready" &&
                               now_playing_metadata_ != "Stopped" &&
                               now_playing_metadata_ != "stopped" &&
                               now_playing_metadata_ != "Playing" &&
                               now_playing_metadata_ != "playing";
      
      if (is_playing_ && has_real_metadata) {
        ESP_LOGD(TAG, "Showing metadata: %s", now_playing_metadata_.c_str());
        std::string display_text = this->format_display_text_(now_playing_metadata_);
        this->display_->set_text(display_text.c_str());  // RetroText auto-scrolls long text
      } else {
        // Show station name (with play or stop icon)
        ESP_LOGD(TAG, "Showing station: %s", browse_items_[currently_playing_index_].name.c_str());
        std::string display_text = this->format_display_text_(browse_items_[currently_playing_index_].name);
        this->display_->set_text(display_text.c_str());
      }
    }
  } else if (this->display_ != nullptr) {
    // No station selected
    std::string display_text = this->format_display_text_(is_playing_ ? "PLAYING" : "STOPPED");
    this->display_->set_text(display_text.c_str());
  }
  
  // Update LEDs to show only playing item
  update_leds_for_browse_();
  
  ESP_LOGI(TAG, "Exited browse mode");
}

void RadioController::scroll_browse_(int direction) {
  if (browse_items_.empty()) {
    return;
  }
  
  // Enter browse mode if not already active
  if (!browse_mode_active_) {
    enter_browse_mode_();
  }
  
  this->last_browse_interaction_ = millis();
  
  // Update index with wrapping
  if (direction > 0) {
    browse_index_ = (browse_index_ + 1) % browse_items_.size();
  } else if (direction < 0) {
    if (browse_index_ == 0) {
      browse_index_ = browse_items_.size() - 1;
    } else {
      browse_index_--;
    }
  }
  
  // Update display
  // Show play/stop icon ONLY if this is the currently playing station
  if (this->display_ != nullptr && browse_index_ < browse_items_.size()) {
    const auto &item = browse_items_[browse_index_];
    std::string display_text;
    
    // Add icon only if this is the currently playing item
    if ((int)browse_index_ == currently_playing_index_) {
      display_text = this->format_display_text_(item.name);
    } else {
      display_text = item.name;
    }
    
    this->display_->set_text(display_text.c_str());
    ESP_LOGI(TAG, "Browsing: %d/%d - %s%s", 
             browse_index_ + 1, browse_items_.size(), 
             ((int)browse_index_ == currently_playing_index_) ? "[PLAYING] " : "",
             item.name.c_str());
  }
  
  update_leds_for_browse_();
}

void RadioController::update_leds_for_browse_() {
  if (!this->panel_leds_initialized_ || !this->led_driver_) {
    return;
  }
  
  // Clear all preset LEDs (7 presets)
  constexpr struct { uint8_t sw; uint8_t cs; } PRESET_LEDS[7] = {
    {3, 3}, {3, 2}, {3, 1}, {3, 0},
    {3, 8}, {3, 7}, {3, 6}
  };
  
  for (uint8_t i = 0; i < 7; i++) {
    this->led_driver_->set_pixel(PRESET_LEDS[i].cs, PRESET_LEDS[i].sw, 0);
  }
  
  if (!browse_mode_active_) {
    // Not browsing - show only the currently playing station at BRIGHT (255)
    if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
      const auto &item = browse_items_[currently_playing_index_];
      if (item.type == BrowseItem::PRESET && item.preset_index >= 0 && item.preset_index < 7) {
        this->led_driver_->set_pixel(PRESET_LEDS[item.preset_index].cs, 
                                     PRESET_LEDS[item.preset_index].sw, 255);  // BRIGHT
      }
    }
  } else {
    // Browsing mode
    // Show currently playing station BRIGHT (255) always
    if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
      const auto &playing = browse_items_[currently_playing_index_];
      if (playing.type == BrowseItem::PRESET && playing.preset_index >= 0 && playing.preset_index < 7) {
        this->led_driver_->set_pixel(PRESET_LEDS[playing.preset_index].cs,
                                     PRESET_LEDS[playing.preset_index].sw, 255);  // BRIGHT
      }
    }
    
    // Show current selection DIM (128) if it's different from playing
    if (browse_index_ < browse_items_.size() && (int)browse_index_ != currently_playing_index_) {
      const auto &current = browse_items_[browse_index_];
      if (current.type == BrowseItem::PRESET && current.preset_index >= 0 && current.preset_index < 7) {
        this->led_driver_->set_pixel(PRESET_LEDS[current.preset_index].cs,
                                     PRESET_LEDS[current.preset_index].sw, 128);  // DIM
      }
    }
  }
  
  // Memory button LED logic:
  // - Bright (255) when in save preset mode OR actively browsing
  // - Dim (64) when playing non-preset station (favorite from MA)
  // - Off when playing a preset slot or stopped
  if (this->save_preset_mode_) {
    // Save mode active - bright
    this->led_driver_->set_pixel(5, 3, 255);
  } else if (browse_mode_active_) {
    // Browsing - bright
    this->led_driver_->set_pixel(5, 3, 255);
  } else if (currently_playing_index_ >= 0 && 
             currently_playing_index_ < (int)browse_items_.size()) {
    const auto &item = browse_items_[currently_playing_index_];
    if (item.type != BrowseItem::PRESET || item.preset_index < 0 || item.preset_index >= 7) {
      // Playing a favorite (not a preset slot) - dim indicator
      this->led_driver_->set_pixel(5, 3, 64);
    } else {
      // Playing a preset - memory LED off
      this->led_driver_->set_pixel(5, 3, 0);
    }
  } else {
    // Stopped - memory LED off
    this->led_driver_->set_pixel(5, 3, 0);
  }
  
  this->led_driver_->show();
}

void RadioController::toggle_play_stop_() {
  // PRIORITY 1: If browsing a DIFFERENT station, play it (even if something else is playing)
  if (browse_mode_active_ && browse_index_ < browse_items_.size() && 
      (int)browse_index_ != currently_playing_index_) {
    ESP_LOGI(TAG, "Encoder button: switching to new station: %s", browse_items_[browse_index_].name.c_str());
    play_browse_item_(browse_index_);
    return;
  }
  
  // PRIORITY 2: If on the current station and playing, toggle stop
  if (is_playing_) {
    ESP_LOGI(TAG, "Encoder button: requesting stop");
    
    is_playing_ = false;
    
    // Update display with stop icon
    if (this->display_ != nullptr) {
      if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
        std::string display_text = this->format_display_text_(browse_items_[currently_playing_index_].name);
        this->display_->set_text(display_text.c_str());
      } else {
        std::string display_text = this->format_display_text_("STOPPED");
        this->display_->set_text(display_text.c_str());
      }
    }
    
    this->set_vu_meter_target_brightness(26);
    this->update_mode_led_(false);
    
    // Signal HA to stop
    if (this->preset_target_sensor_ != nullptr) {
      this->preset_target_sensor_->publish_state("");
    }
    
  } else {
    // PRIORITY 3: If stopped, resume playback
    ESP_LOGD(TAG, "Encoder button: trying to resume (browse_mode=%d, browse_index=%d, currently_playing=%d)",
             browse_mode_active_, browse_index_, currently_playing_index_);
    
    if (browse_mode_active_ && browse_index_ < browse_items_.size()) {
      ESP_LOGI(TAG, "Encoder button: playing selection: %s", browse_items_[browse_index_].name.c_str());
      play_browse_item_(browse_index_);
    } else if (currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
      const auto &item = browse_items_[currently_playing_index_];
      ESP_LOGI(TAG, "Encoder button: resuming: %s (target: %s)", item.name.c_str(), item.target.c_str());
      
      is_playing_ = true;
      
      if (this->display_ != nullptr) {
        std::string display_text = this->format_display_text_(item.name);
        this->display_->set_text(display_text.c_str());
      }
      
      this->update_mode_led_(true);
      this->set_vu_meter_target_brightness(204);
      
      if (this->preset_target_sensor_ != nullptr) {
        this->preset_target_sensor_->publish_state(item.target);
        ESP_LOGD(TAG, "Re-published media_id for resume: '%s'", item.target.c_str());
      }
    } else {
      ESP_LOGW(TAG, "Cannot resume: no valid station (currently_playing_index=%d, browse_items size=%d)",
               currently_playing_index_, browse_items_.size());
    }
  }
}

void RadioController::play_browse_item_(size_t index) {
  if (index >= browse_items_.size()) {
    return;
  }
  
  const auto &item = browse_items_[index];
  
  // Check if this is an empty preset slot or separator
  if (item.target.empty() || item.name.find("---") == 0) {
    ESP_LOGW(TAG, "Cannot play empty or separator item: %s", item.name.c_str());
    if (this->display_ != nullptr) {
      this->display_->set_text("EMPTY SLOT");
      // Stay in browse mode, don't exit
    }
    return;
  }
  
  ESP_LOGI(TAG, "Playing item: %s (target: %s)", item.name.c_str(), item.target.c_str());
  
  // Update state
  currently_playing_index_ = index;
  // Note: browse_index_ stays where user was browsing (don't sync with currently_playing)
  is_playing_ = true;
  
  // Record activation time to delay metadata display
  this->preset_activation_time_ = millis();
  
  // Exit browse mode (will show station name or metadata)
  if (browse_mode_active_) {
    this->exit_browse_mode_();
  }
  
  // Update display with station name (with play icon)
  if (this->display_ != nullptr) {
    std::string display_text = this->format_display_text_(item.name);
    this->display_->set_text(display_text.c_str());
    ESP_LOGD(TAG, "Display updated: '%s' (will show station name for 3 seconds)", display_text.c_str());
  }
  
  // Update text sensors for Home Assistant
  if (this->preset_text_sensor_ != nullptr) {
    this->preset_text_sensor_->publish_state(item.name);
  }
  
  if (this->preset_target_sensor_ != nullptr) {
    this->preset_target_sensor_->publish_state(item.target);
    ESP_LOGD(TAG, "Published media_id: '%s'", item.target.c_str());
  }
  
  // Update LEDs
  update_leds_for_browse_();
  
  // Update mode LED
  this->update_mode_led_(true);
  
  // Set VU meter to playing level
  this->set_vu_meter_target_brightness(204);  // 80% when playing
  
  // The Home Assistant automation will pick up the media_id change and start playback
}

std::string RadioController::format_display_text_(const std::string &text, bool show_icon) {
  if (!show_icon) {
    return text;
  }
  
  // Prepend play (▶ = glyph 128) or stop (⏹ = glyph 129) icon + space
  std::string result;
  result += (char)(is_playing_ ? 128 : 129);  // Play or Stop icon
  result += ' ';
  result += text;
  return result;
}

void RadioController::set_now_playing_metadata(const std::string &metadata) {
  this->now_playing_metadata_ = metadata;
  
  ESP_LOGD(TAG, "Metadata updated: %s", metadata.c_str());
  
  // Filter out non-metadata values like "Ready", "Stopped", "Playing"
  bool is_real_metadata = !metadata.empty() && 
                          metadata != "Ready" && 
                          metadata != "ready" &&
                          metadata != "Stopped" &&
                          metadata != "stopped" &&
                          metadata != "Playing" &&
                          metadata != "playing";
  
  // Check if we're in the "show station name" period (3 seconds after preset activation)
  bool in_station_name_period = false;
  if (this->preset_activation_time_ > 0) {
    uint32_t elapsed = millis() - this->preset_activation_time_;
    in_station_name_period = (elapsed < 3000);  // 3 second delay
    if (in_station_name_period) {
      ESP_LOGD(TAG, "Ignoring metadata - showing station name (%d ms remaining)", 3000 - elapsed);
    }
  }
  
  // If not browsing and currently playing, update display with metadata (if it's real metadata)
  // BUT: Don't override station name for first 3 seconds after preset activation
  if (!browse_mode_active_ && is_playing_ && this->display_ != nullptr && 
      is_real_metadata && !in_station_name_period) {
    std::string display_text = this->format_display_text_(metadata);
    this->display_->set_text(display_text.c_str());  // RetroText auto-scrolls
  }
}

void RadioController::set_playback_state(bool playing) {
  bool state_changed = (is_playing_ != playing);
  is_playing_ = playing;
  
  // Clear activation time when stopping (reset display delay)
  if (!playing) {
    this->preset_activation_time_ = 0;
  }
  
  ESP_LOGI(TAG, "set_playback_state(%s) - state_changed=%d", playing ? "PLAYING" : "STOPPED", state_changed);
  
  // Always update display when called, not just on state change
  // This ensures display updates after spinner clears
  
  // Update VU meter (only if state changed)
  if (state_changed) {
    this->set_vu_meter_target_brightness(playing ? 204 : 26);
    this->update_mode_led_(playing);
  }
  
  // Always update display (even if state didn't change)
  // This ensures the icon appears after loading spinner
  if (!browse_mode_active_ && this->display_ != nullptr) {
    // Filter out non-metadata values
    bool has_real_metadata = !now_playing_metadata_.empty() && 
                             now_playing_metadata_ != "Ready" && 
                             now_playing_metadata_ != "ready" &&
                             now_playing_metadata_ != "Stopped" &&
                             now_playing_metadata_ != "stopped" &&
                             now_playing_metadata_ != "Playing" &&
                             now_playing_metadata_ != "playing";
    
    if (playing && has_real_metadata) {
      // Playing with real metadata - show it with icon
      ESP_LOGD(TAG, "Display: metadata with icon");
      std::string display_text = this->format_display_text_(now_playing_metadata_);
      this->display_->set_text(display_text.c_str());
    } else if (playing && currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
      // Playing but no real metadata yet - show station name with play icon
      ESP_LOGD(TAG, "Display: station name with play icon");
      std::string display_text = this->format_display_text_(browse_items_[currently_playing_index_].name);
      this->display_->set_text(display_text.c_str());
    } else if (!playing && currently_playing_index_ >= 0 && currently_playing_index_ < (int)browse_items_.size()) {
      // Stopped - show station name with stop icon
      ESP_LOGD(TAG, "Display: station name with stop icon");
      std::string display_text = this->format_display_text_(browse_items_[currently_playing_index_].name);
      this->display_->set_text(display_text.c_str());
    } else {
      // Fallback
      ESP_LOGD(TAG, "Display: fallback PLAYING/STOPPED");
      std::string display_text = this->format_display_text_(playing ? "PLAYING" : "STOPPED");
      this->display_->set_text(display_text.c_str());
    }
  }
}

// ====================================================================================
// Preset Storage Management (Flash Persistence)
// ====================================================================================

void RadioController::load_presets_from_flash_() {
  ESP_LOGI(TAG, "Loading presets from flash...");
  
  for (uint8_t i = 0; i < 7; i++) {
    // Create preference object for each slot
    char pref_name[16];
    snprintf(pref_name, sizeof(pref_name), "preset_%d", i);
    uint32_t hash = fnv1_hash(pref_name);
    preset_prefs_[i] = global_preferences->make_preference<StoredPreset>(hash);
    
    // Try to load from flash
    if (preset_prefs_[i].load(&stored_presets_[i])) {
      // Validate loaded data
      if (stored_presets_[i].is_valid) {
        ESP_LOGI(TAG, "Loaded preset %d: %s (%s)", i, 
                 stored_presets_[i].display_name, 
                 stored_presets_[i].media_id);
        
        // Update runtime preset list if we have old-style presets
        if (i < presets_.size()) {
          presets_[i].target = stored_presets_[i].media_id;
          presets_[i].display_text = stored_presets_[i].display_name;
        }
      } else {
        ESP_LOGD(TAG, "Preset slot %d is empty", i);
        snprintf(stored_presets_[i].display_name, sizeof(stored_presets_[i].display_name), 
                 "Empty Slot %d", i + 1);
      }
    } else {
      // First boot or corrupted - initialize as empty
      ESP_LOGW(TAG, "Preset slot %d not found in flash, initializing as empty", i);
      stored_presets_[i].is_valid = false;
      stored_presets_[i].media_id[0] = '\0';
      snprintf(stored_presets_[i].display_name, sizeof(stored_presets_[i].display_name), 
               "Empty Slot %d", i + 1);
      stored_presets_[i].last_played = 0;
    }
  }
}

void RadioController::save_preset_to_slot(uint8_t slot, const std::string &media_id, 
                                           const std::string &display_name) {
  if (slot >= 7) {
    ESP_LOGE(TAG, "Invalid preset slot: %d", slot);
    return;
  }
  
  ESP_LOGI(TAG, "Saving preset to slot %d: %s (%s)", slot, display_name.c_str(), media_id.c_str());
  
  StoredPreset &preset = stored_presets_[slot];
  
  // Update in-memory
  strncpy(preset.media_id, media_id.c_str(), sizeof(preset.media_id) - 1);
  preset.media_id[sizeof(preset.media_id) - 1] = '\0';
  
  strncpy(preset.display_name, display_name.c_str(), sizeof(preset.display_name) - 1);
  preset.display_name[sizeof(preset.display_name) - 1] = '\0';
  
  preset.is_valid = true;
  preset.last_played = millis();
  
  // Save to flash
  this->save_preset_to_flash_(slot);
  
  // Update runtime preset list if it exists
  if (slot < presets_.size()) {
    presets_[slot].target = media_id;
    presets_[slot].display_text = display_name;
  }
  
  // Note: Individual preset slot sensors removed
  // Preset saved successfully - will appear in select component
  
  // Rebuild browse list to reflect changes
  this->build_browse_list_();
  
  // Update select options
  this->update_preset_select_options_();
  
  // Show feedback on display
  char msg[32];
  snprintf(msg, sizeof(msg), "PRESET %d: SAVED", slot + 1);
  if (this->display_) {
    this->display_->set_text(msg);
  }
  
  // Start timer to auto-dismiss message after 2 seconds
  this->preset_saved_message_time_ = millis();
}

void RadioController::save_preset_to_flash_(uint8_t slot) {
  if (slot >= 7) return;
  
  if (preset_prefs_[slot].save(&stored_presets_[slot])) {
    ESP_LOGD(TAG, "Preset %d saved to flash successfully", slot);
  } else {
    ESP_LOGE(TAG, "Failed to save preset %d to flash!", slot);
  }
}

StoredPreset RadioController::get_preset(uint8_t slot) {
  if (slot >= 7) {
    StoredPreset empty = {0};
    return empty;
  }
  return stored_presets_[slot];
}

void RadioController::clear_preset_slot(uint8_t slot) {
  if (slot >= 7) return;
  
  ESP_LOGI(TAG, "Clearing preset slot %d", slot);
  
  StoredPreset &preset = stored_presets_[slot];
  preset.is_valid = false;
  preset.media_id[0] = '\0';
  snprintf(preset.display_name, sizeof(preset.display_name), "Empty Slot %d", slot + 1);
  preset.last_played = 0;
  
  // Save to flash
  this->save_preset_to_flash_(slot);
  
  // Note: Individual preset slot sensors removed
  // Cleared slot will show as empty in select component
  
  // Rebuild browse list
  this->build_browse_list_();
  
  // Update select options
  this->update_preset_select_options_();
}

void RadioController::publish_preset_sensors_() {
  // Preset slot sensors removed - caused API connection issues
  // Instead, presets are exposed via:
  // 1. Select component (lists all 7 presets)
  // 2. current_preset text sensor (shows currently selected)
  // 3. Log messages show preset state
  
  for (uint8_t i = 0; i < 7; i++) {
    if (stored_presets_[i].is_valid) {
      ESP_LOGD(TAG, "Preset %d: %s", i + 1, stored_presets_[i].display_name);
    } else {
      ESP_LOGD(TAG, "Preset %d: Empty", i + 1);
    }
  }
}

void RadioController::update_preset_select_options_() {
  if (!preset_select_) return;
  
  // Build list of options including empty slots
  std::vector<std::string> options;
  for (uint8_t i = 0; i < 7; i++) {
    options.push_back(stored_presets_[i].display_name);
  }
  
  // Update select traits
  // Note: This will be handled by RadioControllerSelect class
  ESP_LOGD(TAG, "Updated select options with %d presets", options.size());
}

void RadioController::register_preset_slot_sensor(uint8_t slot, text_sensor::TextSensor *sensor) {
  // Method kept for API compatibility but no longer used
  // Individual preset sensors caused API connection issues
  ESP_LOGD(TAG, "Note: Preset slot sensors are disabled");
}

// ====================================================================================
// Memory Button Configuration
// ====================================================================================

void RadioController::set_memory_button(uint8_t row, uint8_t column) {
  this->has_memory_button_ = true;
  this->memory_button_row_ = row;
  this->memory_button_col_ = column;
  ESP_LOGCONFIG(TAG, "Memory button configured at row=%d, col=%d", row, column);
}

// ====================================================================================
// Browse All Favorites (Unified Radio + Playlists)
// ====================================================================================

void RadioController::load_all_favorites(const std::string &json_data) {
  ESP_LOGI(TAG, "Loading all favorites from JSON...");
  
  // Parse JSON using ArduinoJson
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json_data);
  
  if (error) {
    ESP_LOGE(TAG, "Failed to parse all favorites JSON: %s", error.c_str());
    return;
  }
  
  // Clear existing favorites
  all_favorites_.clear();
  
  // Parse items array
  if (!doc.is<JsonArray>()) {
    ESP_LOGE(TAG, "All favorites JSON is not an array");
    return;
  }
  
  JsonArray items = doc.as<JsonArray>();
  for (JsonObject item : items) {
    if (item["name"].is<const char*>() && item["uri"].is<const char*>()) {
      PlaylistItem fav;
      fav.name = item["name"].as<std::string>();
      fav.uri = item["uri"].as<std::string>();
      all_favorites_.push_back(fav);
      ESP_LOGD(TAG, "Loaded favorite: %s", fav.name.c_str());
    }
  }
  
  ESP_LOGI(TAG, "Loaded %d favorites from Music Assistant", all_favorites_.size());
  
  // Rebuild browse list to include all favorites
  this->build_browse_list_();
}

}  // namespace radio_controller
}  // namespace esphome
