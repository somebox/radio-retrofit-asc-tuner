#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
  // Use Arduino's millis()
#else
  #include <cstdint>
  #include "platform/Time.h"
  // Use platform::millis() for tests
  using platform::millis;
#endif

namespace Input {

// Button control - tracks press/release state and timing
class ButtonControl {
public:
  enum State { Released = 0, Pressed = 1 };
  
  ButtonControl() 
    : current_(Released)
    , previous_(Released)
    , press_time_(0)
    , change_time_(0) {}
  
  // State changes (called by InputManager)
  // Returns true if state change was applied, false if ignored (invalid transition)
  bool onPress(unsigned long now) {
    // Only accept press if currently released
    if (current_ == Released) {
      previous_ = current_;
      current_ = Pressed;
      press_time_ = now;
      change_time_ = now;
      return true;
    }
    return false;  // Ignore duplicate press
  }
  
  bool onRelease(unsigned long now) {
    // Only accept release if currently pressed
    if (current_ == Pressed) {
      previous_ = current_;
      current_ = Released;
      change_time_ = now;
      return true;
    }
    return false;  // Ignore release when not pressed
  }
  
  // Frame update (for edge detection)
  void update(unsigned long now) {
    previous_ = current_;
  }
  
  // Query API
  bool isPressed() const { 
    return current_ == Pressed; 
  }
  
  bool wasJustPressed() const { 
    return current_ == Pressed && previous_ == Released; 
  }
  
  bool wasJustReleased() const { 
    return current_ == Released && previous_ == Pressed; 
  }
  
  bool isLongPressed(unsigned long now, unsigned long threshold_ms) const {
    return current_ == Pressed && (now - press_time_) >= threshold_ms;
  }
  
  bool wasLongPress(unsigned long threshold_ms) const {
    // Check if the just-released button was held long enough
    return current_ == Released && 
           previous_ == Pressed &&
           (change_time_ - press_time_) >= threshold_ms;
  }
  
  unsigned long pressDuration(unsigned long now) const {
    return isPressed() ? (now - press_time_) : 0;
  }
  
  unsigned long timeSinceChange(unsigned long now) const {
    return now - change_time_;
  }
  
private:
  State current_;
  State previous_;
  unsigned long press_time_;
  unsigned long change_time_;
};

// Encoder control - tracks rotary position and button
class EncoderControl {
public:
  EncoderControl()
    : position_(0)
    , previous_position_(0)
    , last_a_(0)
    , last_b_(0)
    , detent_state_(0)
    , button_() {}
  
  // Called when encoder channel goes HIGH (press event only)
  // Gray code sequence: 00 -> 01 -> 11 -> 10 -> 00 (clockwise)
  // Only counts complete detents (full 4-step cycles)
  void onChannelPress(bool is_a, unsigned long now) {
    updateChannel(is_a, true, now);
  }
  
  // Called when encoder channel goes LOW (release event)
  void onChannelRelease(bool is_a, unsigned long now) {
    updateChannel(is_a, false, now);
  }
  
  // Frame update
  void update(unsigned long now) {
    previous_position_ = position_;
    button_.update(now);
  }
  
  // Query API
  int position() const { return position_; }
  int delta() const { return position_ - previous_position_; }
  bool changed() const { return delta() != 0; }
  
  ButtonControl& button() { return button_; }
  const ButtonControl& button() const { return button_; }
  
  // For debugging
  uint8_t state() const { return (last_a_ << 1) | last_b_; }
  int detentProgress() const { return detent_state_; }
  
private:
  int position_;
  int previous_position_;
  uint8_t last_a_;
  uint8_t last_b_;
  int8_t detent_state_;  // Tracks progress through 4-step detent cycle
  ButtonControl button_;
  
  void updateChannel(bool is_a, bool new_value, unsigned long now) {
    uint8_t current_a = last_a_;
    uint8_t current_b = last_b_;
    
    // Update the channel
    if (is_a) {
      current_a = new_value ? 1 : 0;
    } else {
      current_b = new_value ? 1 : 0;
    }
    
    // Calculate state (2-bit Gray code)
    uint8_t old_state = (last_a_ << 1) | last_b_;
    uint8_t new_state = (current_a << 1) | current_b;
    
    // Detect valid transitions and track detent progress
    if (old_state != new_state) {
      // Clockwise transitions
      if ((old_state == 0b00 && new_state == 0b01) ||
          (old_state == 0b01 && new_state == 0b11) ||
          (old_state == 0b11 && new_state == 0b10) ||
          (old_state == 0b10 && new_state == 0b00)) {
        detent_state_++;
        if (detent_state_ >= 4) {
          position_++;
          detent_state_ = 0;
        }
      }
      // Counter-clockwise transitions  
      else if ((old_state == 0b00 && new_state == 0b10) ||
               (old_state == 0b10 && new_state == 0b11) ||
               (old_state == 0b11 && new_state == 0b01) ||
               (old_state == 0b01 && new_state == 0b00)) {
        detent_state_--;
        if (detent_state_ <= -4) {
          position_--;
          detent_state_ = 0;
        }
      }
      else {
        // Invalid transition - reset detent tracking
        detent_state_ = 0;
      }
      
      // Update state
      last_a_ = current_a;
      last_b_ = current_b;
    }
  }
};

// Multi-position switch control
class SwitchControl {
public:
  SwitchControl(int num_positions = 4)
    : num_positions_(num_positions)
    , current_position_(0)
    , previous_position_(0) {}
  
  void setPosition(int pos, unsigned long now) {
    if (pos >= 0 && pos < num_positions_ && pos != current_position_) {
      previous_position_ = current_position_;
      current_position_ = pos;
    }
  }
  
  void update(unsigned long now) {
    previous_position_ = current_position_;
  }
  
  // Query API
  int position() const { return current_position_; }
  bool changed() const { return current_position_ != previous_position_; }
  int numPositions() const { return num_positions_; }
  
private:
  int num_positions_;
  int current_position_;
  int previous_position_;
};

// Analog control - tracks potentiometer/analog input with change detection
class AnalogControl {
public:
  AnalogControl(int pin = -1, int deadzone = 10, unsigned long min_update_interval_ms = 100)
    : pin_(pin)
    , current_value_(0)
    , previous_value_(0)
    , raw_value_(0)
    , deadzone_(deadzone)
    , min_update_interval_ms_(min_update_interval_ms)
    , last_poll_time_(0)
    , last_change_time_(0) {}
  
  // Called by InputManager to read ADC value
  void poll(unsigned long now) {
    if (pin_ < 0) return;
    
#ifdef ARDUINO
    // Read 12-bit ADC (ESP32 native resolution)
    raw_value_ = analogRead(pin_);
    
    // Apply deadzone and time throttling
    int diff = abs(raw_value_ - current_value_);
    
    // Only update if deadzone threshold is exceeded AND enough time has passed
    // (or this is the first change - last_change_time_ == 0)
    if (diff > deadzone_ && 
        (last_change_time_ == 0 || (now - last_change_time_) >= min_update_interval_ms_)) {
      current_value_ = raw_value_;
      last_change_time_ = now;
    }
#endif
    
    last_poll_time_ = now;
  }
  
  void update(unsigned long now) {
    previous_value_ = current_value_;
  }
  
  // Query API
  int value() const { return current_value_; }
  int rawValue() const { return raw_value_; }
  bool changed() const { return current_value_ != previous_value_; }
  int delta() const { return current_value_ - previous_value_; }
  
  // Convert 12-bit ADC (0-4095) to byte (0-255)
  uint8_t valueAsByte() const {
    return (current_value_ * 255) / 4095;
  }
  
  // Convert to percentage (0-100)
  int valueAsPercent() const {
    return (current_value_ * 100) / 4095;
  }
  
  // Configuration
  void setPin(int pin) { pin_ = pin; }
  void setDeadzone(int deadzone) { deadzone_ = deadzone; }
  void setMinUpdateInterval(unsigned long interval_ms) { min_update_interval_ms_ = interval_ms; }
  int getPin() const { return pin_; }
  
#ifndef ARDUINO
  // Test-only: inject simulated ADC value (native tests only)
  void _test_injectValue(int value, unsigned long now) {
    raw_value_ = value;
    int diff = abs(raw_value_ - current_value_);
    // Match production logic: allow first change or after throttle period
    if (diff > deadzone_ && 
        (last_change_time_ == 0 || (now - last_change_time_) >= min_update_interval_ms_)) {
      current_value_ = raw_value_;
      last_change_time_ = now;
    }
    last_poll_time_ = now;
  }
#endif
  
private:
  int pin_;
  int current_value_;      // Smoothed value after deadzone
  int previous_value_;     // Previous smoothed value
  int raw_value_;          // Raw ADC reading
  int deadzone_;           // Minimum change threshold
  unsigned long min_update_interval_ms_;  // Minimum time between updates
  unsigned long last_poll_time_;
  unsigned long last_change_time_;
};

} // namespace Input

