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

const ButtonControl& InputManager::button(int id) const {
  return buttons_.at(id);
}

const EncoderControl& InputManager::encoder(int id) const {
  return encoders_.at(id);
}

const SwitchControl& InputManager::switch_(int id) const {
  return switches_.at(id);
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
  
  // Second: poll keypad hardware for new events
  if (keypad_) {
    while (keypad_->available()) {
      int event = keypad_->getEvent();
      processKeypadEvent(event);
    }
  }
}

// Process raw keypad event and dispatch to appropriate control
void InputManager::processKeypadEvent(int event) {
  bool pressed = event & 0x80;
  int key_number = (event & 0x7F) - 1;  // Convert to 0-based
  
  // Calculate row/col from key number
  int row = key_number / HardwareConfig::KEYPAD_COLS;
  int col = key_number % HardwareConfig::KEYPAD_COLS;
  
  // Route to appropriate handler
  if (row == HardwareConfig::ENCODER_ROW && 
      col >= HardwareConfig::ENCODER_COL_A && 
      col <= HardwareConfig::ENCODER_COL_BUTTON) {
    handleEncoderEvent(col, pressed);
  }
  else if (row == 0 && col < HardwareConfig::NUM_PRESETS) {
    handleButtonEvent(col, pressed);  // Preset buttons map directly to columns
  }
  // Additional button/control mappings can be added here
}

// Handle preset button events
void InputManager::handleButtonEvent(int button_id, bool pressed) {
  if (buttons_.count(button_id)) {
    if (pressed) {
      buttons_.at(button_id).onPress(current_time_);
    } else {
      buttons_.at(button_id).onRelease(current_time_);
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
