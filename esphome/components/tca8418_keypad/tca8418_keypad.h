/**
 * TCA8418 I2C Keypad Matrix Controller Component for ESPHome
 * 
 * This component provides an interface to the TCA8418 keypad matrix controller.
 * 
 * Based on Adafruit_TCA8418 library (https://github.com/adafruit/Adafruit_TCA8418)
 * Adapted for ESPHome by replacing Adafruit_I2CDevice with ESPHome's I2CDevice.
 * 
 * Original library: Copyright (c) 2020 Adafruit Industries (BSD License)
 * ESPHome adaptation: 2025
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/automation.h"
#include "tca8418_registers.h"
#include <map>

namespace esphome {
namespace tca8418_keypad {

// Forward declarations
class KeyPressTrigger;
class KeyReleaseTrigger;
class TCA8418BinarySensor;

/**
 * TCA8418 Keypad Matrix Controller Component
 * 
 * Handles I2C communication with TCA8418 chip and provides:
 * - Matrix keypad scanning (up to 8x10)
 * - Key event detection (press/release)
 * - ESPHome triggers for automations
 */
class TCA8418Component : public Component, public i2c::I2CDevice {
 public:
  TCA8418Component() = default;

  // Component lifecycle methods
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  float get_loop_priority() const override;

  // Configuration setters (called from Python codegen)
  void set_matrix_size(uint8_t rows, uint8_t columns);
  
  // Trigger registration
  void add_key_press_trigger(KeyPressTrigger *trigger);
  void add_key_release_trigger(KeyReleaseTrigger *trigger);
  
  // Callback registration for direct integration
  using KeyCallback = std::function<void(uint8_t row, uint8_t col, uint8_t key)>;
  void add_on_key_press_callback(KeyCallback callback) {
    this->key_press_callbacks_.push_back(std::move(callback));
  }
  void add_on_key_release_callback(KeyCallback callback) {
    this->key_release_callbacks_.push_back(std::move(callback));
  }
  
  // Binary sensor registration
  void register_key_sensor(uint8_t row, uint8_t col, binary_sensor::BinarySensor *sensor);

 protected:
  // Matrix configuration
  uint8_t rows_{8};
  uint8_t columns_{10};
  
  // Triggers
  std::vector<KeyPressTrigger *> key_press_triggers_;
  std::vector<KeyReleaseTrigger *> key_release_triggers_;
  std::vector<KeyCallback> key_press_callbacks_;
  std::vector<KeyCallback> key_release_callbacks_;
  
  // Binary sensors (map of row*10+col to sensor)
  std::map<uint8_t, binary_sensor::BinarySensor *> key_sensors_;
  
  // I2C register access methods (using ESPHome's I2CDevice)
  bool read_register_(uint8_t reg, uint8_t *value);
  bool write_register_(uint8_t reg, uint8_t value);
  
  // Device initialization helpers
  bool detect_device_();
  bool configure_matrix_();
  bool flush_events_();
  
  // Event processing
  uint8_t available_events_();
  bool read_event_(uint8_t *event);
  void process_event_(uint8_t event);
  void decode_key_event_(uint8_t event, bool *is_press, uint8_t *row, uint8_t *col);
  void fire_key_press_triggers_(uint8_t row, uint8_t col, uint8_t key);
  void fire_key_release_triggers_(uint8_t row, uint8_t col, uint8_t key);
  void update_binary_sensor_(uint8_t row, uint8_t col, bool pressed);
  uint8_t make_sensor_key_(uint8_t row, uint8_t col) const { return row * 10 + col; }
};

// Trigger classes for automation
class KeyPressTrigger : public Trigger<uint8_t, uint8_t, uint8_t> {
 public:
  explicit KeyPressTrigger() = default;
};

class KeyReleaseTrigger : public Trigger<uint8_t, uint8_t, uint8_t> {
 public:
  explicit KeyReleaseTrigger() = default;
};

// Binary sensor for individual keys
class TCA8418BinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(TCA8418Component *parent) { this->parent_ = parent; }
  void set_position(uint8_t row, uint8_t col) {
    this->row_ = row;
    this->col_ = col;
  }
  
  uint8_t get_row() const { return this->row_; }
  uint8_t get_col() const { return this->col_; }
  
  void dump_config() override;

 protected:
  TCA8418Component *parent_{nullptr};
  uint8_t row_{0};
  uint8_t col_{0};
};

}  // namespace tca8418_keypad
}  // namespace esphome
