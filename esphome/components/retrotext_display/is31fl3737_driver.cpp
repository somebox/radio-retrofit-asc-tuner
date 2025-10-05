/**
 * IS31FL3737 LED Matrix Driver Implementation
 */
#include "is31fl3737_driver.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace retrotext_display {

static const char *const TAG = "is31fl3737";

bool IS31FL3737Driver::begin(uint8_t address, i2c::I2CBus *bus) {
  this->address_ = address;
  this->bus_ = bus;
  
  if (this->bus_ == nullptr) {
    ESP_LOGE(TAG, "I2C bus is null");
    return false;
  }
  
  ESP_LOGD(TAG, "Initializing IS31FL3737 at address 0x%02X", this->address_);
  
  // Software reset
  this->reset();
  
  // Enable all LEDs
  if (!this->enable_all_leds_()) {
    ESP_LOGE(TAG, "Failed to enable LEDs");
    return false;
  }
  
  // Configure function page
  if (!this->configure_function_page_()) {
    ESP_LOGE(TAG, "Failed to configure function page");
    return false;
  }
  
  // Clear PWM buffer
  this->clear();
  
  // Switch to PWM page for normal operation
  if (!this->select_page_(IS31FL3737_PAGE_PWM)) {
    ESP_LOGE(TAG, "Failed to select PWM page");
    return false;
  }
  
  this->initialized_ = true;
  ESP_LOGD(TAG, "IS31FL3737 initialized successfully");
  
  return true;
}

void IS31FL3737Driver::reset() {
  // Software reset by reading from reset register
  this->select_page_(IS31FL3737_PAGE_FUNCTION);
  uint8_t dummy;
  this->read_register_(IS31FL3737_REG_RESET, &dummy);
  delay_microseconds_safe(10000);  // Wait 10ms for reset to complete
}

bool IS31FL3737Driver::enable_all_leds_() {
  // Switch to LED Control page
  if (!this->select_page_(IS31FL3737_PAGE_LED_CTRL)) {
    return false;
  }
  
  // LED Control registers are 0x00-0x17 (24 registers)
  // Each register controls 8 LEDs with bitwise mapping
  // Enable all LEDs
  for (uint8_t i = 0x00; i <= 0x17; i++) {
    if (!this->write_register_(i, 0xFF)) {
      ESP_LOGW(TAG, "Failed to write LED control register 0x%02X", i);
      return false;
    }
  }
  
  return true;
}

bool IS31FL3737Driver::configure_function_page_() {
  // Switch to Function page
  if (!this->select_page_(IS31FL3737_PAGE_FUNCTION)) {
    return false;
  }
  
  // Configuration register: SSD=1 (Normal Operation)
  if (!this->write_register_(IS31FL3737_REG_CONFIG, IS31FL3737_CONFIG_SSD)) {
    return false;
  }
  
  // Global Current Control
  if (!this->write_register_(IS31FL3737_REG_GLOBAL_CURRENT, this->global_current_)) {
    return false;
  }
  
  return true;
}

void IS31FL3737Driver::show() {
  if (!this->initialized_ || this->bus_ == nullptr) {
    return;
  }
  
  // Switch to PWM page
  this->select_page_(IS31FL3737_PAGE_PWM);
  
  // Build hardware register buffer (192 bytes for IS31FL3737)
  // IS31FL3737 PWM registers: 12 rows × 16-byte stride = 192 bytes (0x00-0xBF)
  constexpr uint16_t HW_REGISTER_SIZE = IS31FL3737_MATRIX_HEIGHT * IS31FL3737_REGISTER_STRIDE;
  uint8_t hw_buffer[HW_REGISTER_SIZE];
  memset(hw_buffer, 0, sizeof(hw_buffer));
  
  // Map logical buffer to hardware register layout
  for (uint8_t y = 0; y < IS31FL3737_MATRIX_HEIGHT; y++) {
    for (uint8_t x = 0; x < IS31FL3737_MATRIX_WIDTH; x++) {
      uint16_t buffer_index = y * IS31FL3737_MATRIX_WIDTH + x;
      uint16_t reg_address = this->coord_to_register_(x, y);
      hw_buffer[reg_address] = this->pwm_buffer_[buffer_index];
    }
  }
  
  // Write PWM buffer in safe chunks using I2C burst writes with auto-increment
  // ESP32 I2C buffer is typically 128 bytes, so use 64-byte data chunks (+ 1 byte for register address)
  constexpr uint8_t CHUNK_SIZE = 64;
  uint16_t bytes_remaining = HW_REGISTER_SIZE;
  uint16_t bytes_written = 0;
  
  while (bytes_remaining > 0) {
    uint8_t bytes_to_write = (bytes_remaining > CHUNK_SIZE) ? CHUNK_SIZE : bytes_remaining;
    uint8_t chunk_buffer[CHUNK_SIZE + 1];
    
    // First byte is the starting register address
    chunk_buffer[0] = bytes_written;
    
    // Copy chunk data
    memcpy(&chunk_buffer[1], &hw_buffer[bytes_written], bytes_to_write);
    
    // Write chunk using I2C burst mode (auto-increment)
    this->bus_->write(this->address_, chunk_buffer, bytes_to_write + 1);
    
    bytes_written += bytes_to_write;
    bytes_remaining -= bytes_to_write;
  }
}

void IS31FL3737Driver::clear() {
  this->pwm_buffer_.fill(0);
}

void IS31FL3737Driver::set_pixel(uint8_t x, uint8_t y, uint8_t brightness) {
  // Bounds check
  if (x >= IS31FL3737_MATRIX_WIDTH || y >= IS31FL3737_MATRIX_HEIGHT) {
    return;
  }
  
  uint16_t index = y * IS31FL3737_MATRIX_WIDTH + x;
  this->pwm_buffer_[index] = brightness;
}

uint8_t IS31FL3737Driver::get_pixel(uint8_t x, uint8_t y) const {
  // Bounds check
  if (x >= IS31FL3737_MATRIX_WIDTH || y >= IS31FL3737_MATRIX_HEIGHT) {
    return 0;
  }
  
  uint16_t index = y * IS31FL3737_MATRIX_WIDTH + x;
  return this->pwm_buffer_[index];
}

void IS31FL3737Driver::set_global_current(uint8_t current) {
  this->global_current_ = current;
  
  if (this->initialized_) {
    // Update hardware
    this->select_page_(IS31FL3737_PAGE_FUNCTION);
    this->write_register_(IS31FL3737_REG_GLOBAL_CURRENT, current);
    this->select_page_(IS31FL3737_PAGE_PWM);  // Switch back to PWM page
  }
}

bool IS31FL3737Driver::select_page_(uint8_t page) {
  if (this->bus_ == nullptr) {
    return false;
  }
  
  // Unlock command register
  uint8_t unlock_buffer[2] = {IS31FL3737_REG_UNLOCK, IS31FL3737_UNLOCK_VALUE};
  if (this->bus_->write(this->address_, unlock_buffer, 2) != i2c::ERROR_OK) {
    return false;
  }
  
  // Select page
  uint8_t page_buffer[2] = {IS31FL3737_REG_COMMAND, page};
  return this->bus_->write(this->address_, page_buffer, 2) == i2c::ERROR_OK;
}

bool IS31FL3737Driver::write_register_(uint8_t reg, uint8_t value) {
  if (this->bus_ == nullptr) {
    return false;
  }
  
  uint8_t buffer[2] = {reg, value};
  return this->bus_->write(this->address_, buffer, 2) == i2c::ERROR_OK;
}

bool IS31FL3737Driver::read_register_(uint8_t reg, uint8_t *value) {
  if (this->bus_ == nullptr || value == nullptr) {
    return false;
  }
  
  // Write register address
  if (this->bus_->write(this->address_, &reg, 1) != i2c::ERROR_OK) {
    return false;
  }
  
  // Read value
  return this->bus_->read(this->address_, value, 1) == i2c::ERROR_OK;
}

uint16_t IS31FL3737Driver::coord_to_register_(uint8_t x, uint8_t y) const {
  // Convert 0-based coordinates to 1-based CS/SW
  uint8_t cs = x + 1;  // CSx (1-12)
  uint8_t sw = y + 1;  // SWy (1-12)
  
  // IS31FL3737 hardware quirk: CS7-CS12 (1-based) are remapped to CS9-CS14 in register space
  // This is documented in the IS31FL373x driver library (line 365-368)
  if (cs >= 7 && cs <= 12) {
    cs = cs + 2;  // CS7→CS9, CS8→CS10, ..., CS12→CS14
  }
  
  // IS31FL3737 register mapping: Address = (SWy - 1) * stride + (CSx - 1)
  return static_cast<uint16_t>((sw - 1) * IS31FL3737_REGISTER_STRIDE + (cs - 1));
}

}  // namespace retrotext_display
}  // namespace esphome
