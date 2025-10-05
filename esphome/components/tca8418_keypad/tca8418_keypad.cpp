/**
 * TCA8418 I2C Keypad Matrix Controller Component for ESPHome
 * 
 * Implementation file - I2C communication and device detection
 */
#include "tca8418_keypad.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tca8418_keypad {

static const char *const TAG = "tca8418_keypad";

void TCA8418Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TCA8418 Keypad...");
  
  // Detect device on I2C bus
  if (!this->detect_device_()) {
    ESP_LOGE(TAG, "Failed to detect TCA8418 device at address 0x%02X", this->address_);
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "TCA8418 device detected successfully");
  
  // Configure the matrix
  if (!this->configure_matrix_()) {
    ESP_LOGE(TAG, "Failed to configure matrix");
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "Matrix configured: %dx%d", this->rows_, this->columns_);
  
  // Flush any pending events from before initialization
  this->flush_events_();
  
  // Device is configured and ready
  ESP_LOGI(TAG, "TCA8418 initialization complete");
}

void TCA8418Component::loop() {
  // Poll for key events
  // Check if any events are available in the FIFO
  uint8_t event_count = this->available_events_();
  
  if (event_count > 0) {
    // Process all available events
    for (uint8_t i = 0; i < event_count && i < 10; i++) {  // Limit to 10 events per loop
      uint8_t event = 0;
      if (this->read_event_(&event)) {
        this->process_event_(event);
      }
    }
  }
}

void TCA8418Component::dump_config() {
  ESP_LOGCONFIG(TAG, "TCA8418 Keypad Matrix Controller:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Matrix Size: %d rows x %d columns", this->rows_, this->columns_);
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with TCA8418 failed!");
  } else {
    ESP_LOGCONFIG(TAG, "  Status: Device detected and ready");
  }
}

float TCA8418Component::get_setup_priority() const {
  // Initialize after I2C bus is ready, before most other components
  return setup_priority::DATA;
}

void TCA8418Component::set_matrix_size(uint8_t rows, uint8_t columns) {
  this->rows_ = rows;
  this->columns_ = columns;
}

void TCA8418Component::add_key_press_trigger(KeyPressTrigger *trigger) {
  this->key_press_triggers_.push_back(trigger);
}

void TCA8418Component::add_key_release_trigger(KeyReleaseTrigger *trigger) {
  this->key_release_triggers_.push_back(trigger);
}

void TCA8418Component::register_key_sensor(uint8_t row, uint8_t col, binary_sensor::BinarySensor *sensor) {
  uint8_t key = this->make_sensor_key_(row, col);
  this->key_sensors_[key] = sensor;
  ESP_LOGD(TAG, "Registered binary sensor for row=%d, col=%d (key=%d)", row, col, key);
}

// I2C Communication Methods

bool TCA8418Component::read_register_(uint8_t reg, uint8_t *value) {
  // Use ESPHome's I2CDevice read_register method
  // This handles the I2C transaction: write register address, read data byte
  return this->read_register(reg, value, 1) == i2c::ERROR_OK;
}

bool TCA8418Component::write_register_(uint8_t reg, uint8_t value) {
  // Use ESPHome's I2CDevice write_register method
  // This handles the I2C transaction: write register address + data byte
  return this->write_register(reg, &value, 1) == i2c::ERROR_OK;
}

bool TCA8418Component::detect_device_() {
  // Try to read the configuration register to verify device presence
  uint8_t cfg_value = 0;
  
  // Try to read configuration register
  if (!this->read_register_(TCA8418_REG_CFG, &cfg_value)) {
    ESP_LOGW(TAG, "Failed to read configuration register - device may not be present");
    return false;
  }
  
  ESP_LOGD(TAG, "Read CFG register: 0x%02X", cfg_value);
  
  // Try to read interrupt status register (another basic register)
  uint8_t int_stat = 0;
  if (!this->read_register_(TCA8418_REG_INT_STAT, &int_stat)) {
    ESP_LOGW(TAG, "Failed to read interrupt status register");
    return false;
  }
  
  ESP_LOGD(TAG, "Read INT_STAT register: 0x%02X", int_stat);
  
  // If we can read registers, device is present
  ESP_LOGD(TAG, "TCA8418 device communication verified");
  return true;
}

bool TCA8418Component::configure_matrix_() {
  // Based on Adafruit_TCA8418::matrix() implementation
  // Configure which pins are rows (first N pins) and columns (next M pins)
  
  if ((this->rows_ > 8) || (this->columns_ > 10)) {
    ESP_LOGE(TAG, "Invalid matrix size: %dx%d (max 8x10)", this->rows_, this->columns_);
    return false;
  }
  
  // Set default GPIO state - all pins as inputs
  if (!this->write_register_(TCA8418_REG_GPIO_DIR_1, 0x00) ||
      !this->write_register_(TCA8418_REG_GPIO_DIR_2, 0x00) ||
      !this->write_register_(TCA8418_REG_GPIO_DIR_3, 0x00)) {
    ESP_LOGE(TAG, "Failed to set GPIO direction");
    return false;
  }
  
  // Enable GPI event mode for all pins
  if (!this->write_register_(TCA8418_REG_GPI_EM_1, 0xFF) ||
      !this->write_register_(TCA8418_REG_GPI_EM_2, 0xFF) ||
      !this->write_register_(TCA8418_REG_GPI_EM_3, 0xFF)) {
    ESP_LOGE(TAG, "Failed to enable GPI event mode");
    return false;
  }
  
  // Configure keypad matrix size
  // KP_GPIO registers: 1 = keypad pin, 0 = GPIO pin
  // Register 1 = ROW0-ROW7 (bits 0-7)
  // Register 2 = COL0-COL7 (bits 0-7)
  // Register 3 = COL8-COL9 (bits 0-1)
  
  // Build row mask (first N pins are rows)
  uint8_t row_mask = 0;
  for (int r = 0; r < this->rows_; r++) {
    row_mask |= (1 << r);
  }
  
  // Build column masks (next M pins are columns)
  uint8_t col_mask_low = 0;  // COL0-COL7
  uint8_t col_mask_high = 0; // COL8-COL9
  
  for (int c = 0; c < this->columns_ && c < 8; c++) {
    col_mask_low |= (1 << c);
  }
  
  if (this->columns_ > 8) {
    for (int c = 8; c < this->columns_; c++) {
      col_mask_high |= (1 << (c - 8));
    }
  }
  
  // Write keypad/GPIO selection registers
  if (!this->write_register_(TCA8418_REG_KP_GPIO_1, row_mask)) {
    ESP_LOGE(TAG, "Failed to set row mask");
    return false;
  }
  
  if (!this->write_register_(TCA8418_REG_KP_GPIO_2, col_mask_low)) {
    ESP_LOGE(TAG, "Failed to set column mask (low)");
    return false;
  }
  
  if (this->columns_ > 8) {
    if (!this->write_register_(TCA8418_REG_KP_GPIO_3, col_mask_high)) {
      ESP_LOGE(TAG, "Failed to set column mask (high)");
      return false;
    }
  }
  
  // Enable key event interrupts
  uint8_t cfg = TCA8418_REG_CFG_KE_IEN;  // Enable key event interrupts
  if (!this->write_register_(TCA8418_REG_CFG, cfg)) {
    ESP_LOGE(TAG, "Failed to enable key event interrupts");
    return false;
  }
  
  ESP_LOGD(TAG, "Matrix configured: rows=0x%02X, cols_low=0x%02X, cols_high=0x%02X", 
           row_mask, col_mask_low, col_mask_high);
  
  return true;
}

bool TCA8418Component::flush_events_() {
  // Flush all events from FIFO buffer
  // Based on Adafruit_TCA8418::flush()
  
  int count = 0;
  uint8_t event = 0;
  
  // Read and discard all events in FIFO
  while (this->read_register_(TCA8418_REG_KEY_EVENT_A, &event) && event != 0 && count < 100) {
    ESP_LOGV(TAG, "Flushed event: 0x%02X", event);
    count++;
  }
  
  // Clear interrupt status bits
  if (!this->write_register_(TCA8418_REG_INT_STAT, 0x03)) {
    ESP_LOGW(TAG, "Failed to clear interrupt status");
    return false;
  }
  
  if (count > 0) {
    ESP_LOGI(TAG, "Flushed %d pending events", count);
  }
  
  return true;
}

// Event Processing Methods

uint8_t TCA8418Component::available_events_() {
  // Read the event counter from KEY_LCK_EC register
  // Lower 4 bits contain the event count
  uint8_t count_reg = 0;
  if (!this->read_register_(TCA8418_REG_KEY_LCK_EC, &count_reg)) {
    return 0;
  }
  
  return count_reg & 0x0F;  // Mask to get only the event count bits
}

bool TCA8418Component::read_event_(uint8_t *event) {
  // Read one event from the FIFO (KEY_EVENT_A register)
  // Reading this register automatically advances the FIFO
  return this->read_register_(TCA8418_REG_KEY_EVENT_A, event);
}

void TCA8418Component::process_event_(uint8_t event) {
  // Decode and log the event
  if (event == 0) {
    return;  // No event
  }
  
  bool is_press = false;
  uint8_t row = 0;
  uint8_t col = 0;
  
  this->decode_key_event_(event, &is_press, &row, &col);
  
  // Log the key event
  const char *action = is_press ? "PRESS" : "RELEASE";
  ESP_LOGI(TAG, "Key %s: row=%d, col=%d (event=0x%02X)", action, row, col, event);
  
  // Calculate key code (1-based: row * 10 + col + 1)
  uint8_t key_code = (row * 10) + col + 1;
  
  // Update binary sensor state
  this->update_binary_sensor_(row, col, is_press);
  
  // Fire ESPHome triggers
  if (is_press) {
    this->fire_key_press_triggers_(row, col, key_code);
  } else {
    this->fire_key_release_triggers_(row, col, key_code);
  }
}

void TCA8418Component::decode_key_event_(uint8_t event, bool *is_press, uint8_t *row, uint8_t *col) {
  // Event byte format (from TCA8418 hardware testing and Adafruit examples):
  // WARNING: Adafruit_TCA8418.h line 71 comment is BACKWARDS!
  //
  // Event byte structure (CORRECT interpretation from hardware testing):
  //   Bit 7: Press/Release flag
  //     1 = PRESS  (events 0x81-0xD0 for matrix, 0xDB-0xF2 for GPIO)
  //     0 = RELEASE (events 0x01-0x50 for matrix, 0x5B-0x72 for GPIO)
  //   Bits 6-0: Key code (1-based position in matrix)
  //
  // Key code encoding for matrix keys:
  //   Formula: key_code = (row * 10) + col + 1
  //   Range: 0x01-0x50 (1-80 decimal)
  //   Example: Row 3, Col 3 = (3 * 10) + 3 + 1 = 34 = 0x22
  //     Press event: 0xA2 (0x22 | 0x80, bit 7 = 1)
  //     Release event: 0x22 (bit 7 = 0)
  //
  // IMPORTANT: Bit 7 = 1 means PRESS, bit 7 = 0 means RELEASE
  // This is confirmed by actual hardware testing and Adafruit example code.
  // Note: The library header comment is BACKWARDS - trust the example code!
  
  *is_press = (event & 0x80) != 0;  // Bit 7 = 1 means press, bit 7 = 0 means release
  
  uint8_t key_code = event & 0x7F;  // Remove press/release bit (mask off bit 7)
  
  // Decode matrix position from key code
  // Key codes start at 1, so subtract 1 to get 0-based index
  if (key_code > 0 && key_code <= 0x50) {
    key_code--;  // Convert to 0-based (0-79)
    *row = key_code / 10;  // Integer division gives row (0-7)
    *col = key_code % 10;  // Remainder gives column (0-9)
  } else {
    // Invalid or GPIO event (0x5B-0x72 range not used in matrix mode)
    *row = 0xFF;
    *col = 0xFF;
  }
}

void TCA8418Component::fire_key_press_triggers_(uint8_t row, uint8_t col, uint8_t key) {
  for (auto *trigger : this->key_press_triggers_) {
    trigger->trigger(row, col, key);
  }
  for (auto &callback : this->key_press_callbacks_) {
    callback(row, col, key);
  }
}

void TCA8418Component::fire_key_release_triggers_(uint8_t row, uint8_t col, uint8_t key) {
  for (auto *trigger : this->key_release_triggers_) {
    trigger->trigger(row, col, key);
  }
  for (auto &callback : this->key_release_callbacks_) {
    callback(row, col, key);
  }
}

void TCA8418Component::update_binary_sensor_(uint8_t row, uint8_t col, bool pressed) {
  uint8_t key = this->make_sensor_key_(row, col);
  auto it = this->key_sensors_.find(key);
  if (it != this->key_sensors_.end()) {
    it->second->publish_state(pressed);
  }
}

// TCA8418BinarySensor implementation
void TCA8418BinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "TCA8418 Binary Sensor", this);
  ESP_LOGCONFIG(TAG, "  Row: %d", this->row_);
  ESP_LOGCONFIG(TAG, "  Column: %d", this->col_);
}

}  // namespace tca8418_keypad
}  // namespace esphome
