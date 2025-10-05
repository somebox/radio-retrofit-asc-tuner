/**
 * Panel LEDs Component Implementation
 */
#include "panel_leds.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panel_leds {

static const char *const TAG = "panel_leds";

// LED positions from HardwareConfig.h
struct LEDPosition {
  uint8_t sw;  // Row
  uint8_t cs;  // Column
};

// Preset LEDs (8 presets)
static const LEDPosition PRESET_LEDS[8] = {
  {3, 3},  // Preset 1
  {3, 2},  // Preset 2
  {3, 1},  // Preset 3
  {3, 0},  // Preset 4
  {3, 8},  // Preset 5
  {3, 7},  // Preset 6
  {3, 6},  // Preset 7
  {3, 5}   // Memory/Preset 8
};

// Mode LEDs (4 modes)
static const LEDPosition MODE_LEDS[4] = {
  {0, 7},  // Stereo
  {0, 6},  // Stereo-Far
  {0, 8},  // Q
  {0, 5}   // Mono
};

void PanelLEDs::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Panel LEDs...");
  
  // Initialize IS31FL3737 driver (shared with display component)
  this->driver_.reset(new IS31FL3737Driver());
  
  if (!this->driver_->begin(this->address_, this->i2c_bus_)) {
    ESP_LOGE(TAG, "Failed to initialize IS31FL3737 at address 0x%02X", this->address_);
    this->mark_failed();
    return;
  }
  
  // Set global current
  this->driver_->set_global_current(this->brightness_);
  
  // Clear all LEDs
  this->clear_all();
  
  ESP_LOGCONFIG(TAG, "Panel LEDs initialized successfully");
}

void PanelLEDs::loop() {
  // Nothing to do in loop - LEDs are set on-demand
}

void PanelLEDs::dump_config() {
  ESP_LOGCONFIG(TAG, "Panel LEDs:");
  ESP_LOGCONFIG(TAG, "  I2C Address: 0x%02X", this->address_);
  ESP_LOGCONFIG(TAG, "  Brightness: %d", this->brightness_);
  ESP_LOGCONFIG(TAG, "  Preset LEDs: 8");
  ESP_LOGCONFIG(TAG, "  Mode LEDs: 4");
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  FAILED - Communication error");
  }
}

float PanelLEDs::get_setup_priority() const {
  // Initialize after I2C
  return setup_priority::DATA;
}

void PanelLEDs::set_preset_led(uint8_t preset_index, bool on) {
  if (preset_index >= 8) {
    return;
  }
  
  const LEDPosition &led = PRESET_LEDS[preset_index];
  uint8_t brightness = on ? this->brightness_ : 0;
  this->driver_->set_pixel(led.cs, led.sw, brightness);
  this->driver_->show();
  
  ESP_LOGD(TAG, "Preset LED %d: %s", preset_index, on ? "ON" : "OFF");
}

void PanelLEDs::set_all_preset_leds(bool on) {
  for (uint8_t i = 0; i < 8; i++) {
    const LEDPosition &led = PRESET_LEDS[i];
    uint8_t brightness = on ? this->brightness_ : 0;
    this->driver_->set_pixel(led.cs, led.sw, brightness);
  }
  this->driver_->show();
  
  ESP_LOGD(TAG, "All preset LEDs: %s", on ? "ON" : "OFF");
}

void PanelLEDs::set_active_preset(uint8_t preset_index) {
  if (preset_index >= 8) {
    return;
  }
  
  // Turn off all preset LEDs first
  for (uint8_t i = 0; i < 8; i++) {
    const LEDPosition &led = PRESET_LEDS[i];
    this->driver_->set_pixel(led.cs, led.sw, 0);
  }
  
  // Turn on only the active preset
  const LEDPosition &led = PRESET_LEDS[preset_index];
  this->driver_->set_pixel(led.cs, led.sw, this->brightness_);
  this->driver_->show();
  
  this->active_preset_ = preset_index;
  ESP_LOGI(TAG, "Active preset: %d", preset_index);
}

void PanelLEDs::set_mode_led(uint8_t mode_index, bool on) {
  if (mode_index >= 4) {
    return;
  }
  
  const LEDPosition &led = MODE_LEDS[mode_index];
  uint8_t brightness = on ? this->brightness_ : 0;
  this->driver_->set_pixel(led.cs, led.sw, brightness);
  this->driver_->show();
  
  ESP_LOGD(TAG, "Mode LED %d: %s", mode_index, on ? "ON" : "OFF");
}

void PanelLEDs::set_all_mode_leds(bool on) {
  for (uint8_t i = 0; i < 4; i++) {
    const LEDPosition &led = MODE_LEDS[i];
    uint8_t brightness = on ? this->brightness_ : 0;
    this->driver_->set_pixel(led.cs, led.sw, brightness);
  }
  this->driver_->show();
  
  ESP_LOGD(TAG, "All mode LEDs: %s", on ? "ON" : "OFF");
}

void PanelLEDs::clear_all() {
  this->driver_->clear();
  this->driver_->show();
  this->active_preset_ = 255;
  ESP_LOGD(TAG, "Cleared all LEDs");
}

}  // namespace panel_leds
}  // namespace esphome
