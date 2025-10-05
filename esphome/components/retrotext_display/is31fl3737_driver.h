/**
 * IS31FL3737 LED Matrix Driver for ESPHome
 * 
 * Simplified driver for IS31FL3737 12×12 matrix LED controller.
 * Adapted from IS31FL373x Driver library for ESPHome I2C integration.
 * 
 * Based on: https://github.com/somebox/IS31FL373x-Driver
 * Original copyright (c) 2024 Jeremy Kenyon, MIT License
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "is31fl3737_registers.h"
#include <array>

namespace esphome {
namespace retrotext_display {

/**
 * Driver for a single IS31FL3737 chip (12×12 matrix)
 */
class IS31FL3737Driver {
 public:
  IS31FL3737Driver() = default;
  ~IS31FL3737Driver() = default;

  // Initialization
  bool begin(uint8_t address, i2c::I2CBus *bus);
  void reset();

  // Display control
  void show();  // Push buffer to hardware
  void clear(); // Clear buffer

  // Pixel operations
  void set_pixel(uint8_t x, uint8_t y, uint8_t brightness);
  uint8_t get_pixel(uint8_t x, uint8_t y) const;

  // Configuration
  void set_global_current(uint8_t current);
  
  // Status
  bool is_initialized() const { return initialized_; }
  uint8_t get_address() const { return address_; }

 protected:
  // I2C bus and address
  i2c::I2CBus *bus_{nullptr};
  uint8_t address_{0};
  bool initialized_{false};
  
  // PWM buffer (12×12 = 144 pixels)
  std::array<uint8_t, IS31FL3737_PWM_BUFFER_SIZE> pwm_buffer_;
  
  // Global current setting
  uint8_t global_current_{128};

  // Low-level I2C operations
  bool select_page_(uint8_t page);
  bool write_register_(uint8_t reg, uint8_t value);
  bool read_register_(uint8_t reg, uint8_t *value);
  
  // Coordinate mapping
  uint16_t coord_to_register_(uint8_t x, uint8_t y) const;
  
  // Helper methods
  bool enable_all_leds_();
  bool configure_function_page_();
};

}  // namespace retrotext_display
}  // namespace esphome
