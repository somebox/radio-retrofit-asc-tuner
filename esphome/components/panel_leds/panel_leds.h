/**
 * Panel LEDs Component for ESPHome
 * 
 * Controls IS31FL3737 LED matrix for preset buttons and mode indicators.
 * Hardware: IS31FL3737 at I2C address 0x55
 * 
 * Uses the shared IS31FL3737Driver from retrotext_display component.
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/retrotext_display/is31fl3737_driver.h"
#include <memory>

namespace esphome {
namespace panel_leds {

// Use the shared driver from retrotext_display
using retrotext_display::IS31FL3737Driver;

class PanelLEDs : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // Configuration
  void set_i2c_bus(i2c::I2CBus *bus) { this->i2c_bus_ = bus; }
  void set_brightness(uint8_t brightness) { this->brightness_ = brightness; }

  // Public API
  void set_preset_led(uint8_t preset_index, bool on);
  void set_all_preset_leds(bool on);
  void set_active_preset(uint8_t preset_index);
  void set_mode_led(uint8_t mode_index, bool on);
  void set_all_mode_leds(bool on);
  void clear_all();

 protected:
  i2c::I2CBus *i2c_bus_{nullptr};
  uint8_t brightness_{128};
  std::unique_ptr<IS31FL3737Driver> driver_;
  uint8_t active_preset_{255};  // 255 = none active
};

}  // namespace panel_leds
}  // namespace esphome
