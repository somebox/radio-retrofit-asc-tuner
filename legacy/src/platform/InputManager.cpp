#include "platform/InputManager.h"
#include "hardware/HardwareConfig.h"

namespace Input {

InputManager::InputManager()
  : keypad_(nullptr)
  , current_time_(0) {
}

// Registration methods
void InputManager::registerButton(int id) {
  buttons_.emplace(id, ButtonControl());
}

void InputManager::registerEncoder(int id) {
  encoders_.emplace(id, EncoderControl());
}

void InputManager::registerSwitch(int id, int num_positions) {
  switches_.emplace(id, SwitchControl(num_positions));
}

void InputManager::registerAnalog(int id, int pin, int deadzone, unsigned long min_update_interval_ms) {
  analogs_.emplace(id, AnalogControl(pin, deadzone, min_update_interval_ms));
}

// Query API
ButtonControl& InputManager::button(int id) {
  return buttons_.at(id);
}

EncoderControl& InputManager::encoder(int id) {
  return encoders_.at(id);
}

SwitchControl& InputManager::switch_(int id) {
  return switches_.at(id);
}

AnalogControl& InputManager::analog(int id) {
  return analogs_.at(id);
}

const ButtonControl& InputManager::button(int id) const {
  return buttons_.at(id);
}

const EncoderControl& InputManager::encoder(int id) const {
  return encoders_.at(id);
}

const SwitchControl& InputManager::switch_(int id) const {
  return switches_.at(id);
}

const AnalogControl& InputManager::analog(int id) const {
  return analogs_.at(id);
}

// Main update loop
void InputManager::update() {
  current_time_ = millis();
  
  // First: save previous state for all controls (start of frame)
  for (auto& pair : buttons_) {
    pair.second.update(current_time_);
  }
  for (auto& pair : encoders_) {
    pair.second.update(current_time_);
  }
  for (auto& pair : switches_) {
    pair.second.update(current_time_);
  }
  for (auto& pair : analogs_) {
    pair.second.update(current_time_);
  }
  
  // Second: poll keypad hardware for new events
  if (keypad_) {
    while (keypad_->available()) {
      int event = keypad_->getEvent();
      processKeypadEvent(event);
    }
  }
  
  // Third: poll analog inputs to read new values
  for (auto& pair : analogs_) {
    pair.second.poll(current_time_);
  }
}

// Process raw keypad event and dispatch to appropriate control
void InputManager::processKeypadEvent(int event) {
  // TCA8418 format: bit 7 = 1 (press), bit 7 = 0 (release)
  // Per Adafruit example and datasheet Table 1
  bool pressed = (event & 0x80) != 0;
  int key_number = (event & 0x7F) - 1;  // Convert to 0-based
  
  // Calculate row/col from key number
  int row = key_number / HardwareConfig::KEYPAD_COLS;
  int col = key_number % HardwareConfig::KEYPAD_COLS;
  
  Serial.printf("[InputManager] Raw event: 0x%02X → row=%d, col=%d, %s\n", 
                event, row, col, pressed ? "PRESS" : "RELEASE");
  
  // Route to appropriate handler
  if (row == HardwareConfig::ENCODER_ROW && 
      col >= HardwareConfig::ENCODER_COL_A && 
      col <= HardwareConfig::ENCODER_COL_BUTTON) {
    handleEncoderEvent(col, pressed);
  }
  else if (row == HardwareConfig::PRESET_BUTTON_ROW) {
    // Find which preset button this is based on column
    int found_index = -1;
    for (int i = 0; i < HardwareConfig::NUM_PRESETS; i++) {
      if (col == HardwareConfig::PRESET_BUTTONS[i].col) {
        found_index = i;
        Serial.printf("[InputManager] Matched preset: col %d → index %d (%s)\n",
                      col, i, HardwareConfig::PRESET_BUTTONS[i].name);
        handleButtonEvent(i, pressed);
        break;
      }
    }
    if (found_index == -1) {
      Serial.printf("[InputManager] WARNING: No preset found for row=%d, col=%d\n", row, col);
    }
  }
  // Additional button/control mappings can be added here
}

// Handle preset button events
void InputManager::handleButtonEvent(int button_id, bool pressed) {
  if (buttons_.count(button_id)) {
    bool accepted = false;
    if (pressed) {
      accepted = buttons_.at(button_id).onPress(current_time_);
    } else {
      accepted = buttons_.at(button_id).onRelease(current_time_);
    }
    
    if (!accepted) {
      Serial.printf("[InputManager] ⚠️  Ignored invalid transition for button %d (%s)\n",
                    button_id, pressed ? "PRESS on already pressed" : "RELEASE on not pressed");
    }
  }
}

// Handle encoder events (channels A, B, and button)
void InputManager::handleEncoderEvent(int col, bool pressed) {
  if (!encoders_.count(0)) {
    return;  // Encoder not registered
  }
  
  EncoderControl& enc = encoders_.at(0);
  
  if (col == HardwareConfig::ENCODER_COL_A || col == HardwareConfig::ENCODER_COL_B) {
    // Quadrature channels A and B
    bool is_a = (col == HardwareConfig::ENCODER_COL_A);
    if (pressed) {
      enc.onChannelPress(is_a, current_time_);
    } else {
      enc.onChannelRelease(is_a, current_time_);
    }
  }
  else if (col == HardwareConfig::ENCODER_COL_BUTTON) {
    // Encoder button
    if (pressed) {
      enc.button().onPress(current_time_);
    } else {
      enc.button().onRelease(current_time_);
    }
  }
}

} // namespace Input
