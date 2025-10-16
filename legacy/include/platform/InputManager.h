#pragma once

#include <Arduino.h>
#include <map>
#include <Adafruit_TCA8418.h>
#include "platform/InputControls.h"

namespace Input {

class InputManager {
public:
  InputManager();
  ~InputManager() = default;
  
  // Setup
  void setKeypad(Adafruit_TCA8418* keypad) { keypad_ = keypad; }
  
  // Registration (call during setup)
  void registerButton(int id);
  void registerEncoder(int id);
  void registerSwitch(int id, int num_positions = 4);
  void registerAnalog(int id, int pin, int deadzone = 10, unsigned long min_update_interval_ms = 100);
  
  // Update (call every frame)
  void update();
  
  // Query API
  ButtonControl& button(int id);
  EncoderControl& encoder(int id);
  SwitchControl& switch_(int id);
  AnalogControl& analog(int id);
  
  const ButtonControl& button(int id) const;
  const EncoderControl& encoder(int id) const;
  const SwitchControl& switch_(int id) const;
  const AnalogControl& analog(int id) const;
  
  // Check existence
  bool hasButton(int id) const { return buttons_.count(id) > 0; }
  bool hasEncoder(int id) const { return encoders_.count(id) > 0; }
  bool hasSwitch(int id) const { return switches_.count(id) > 0; }
  bool hasAnalog(int id) const { return analogs_.count(id) > 0; }
  
  // Get current time (useful for queries)
  unsigned long currentTime() const { return current_time_; }
  
private:
  // Hardware
  Adafruit_TCA8418* keypad_;
  
  // Controls
  std::map<int, ButtonControl> buttons_;
  std::map<int, EncoderControl> encoders_;
  std::map<int, SwitchControl> switches_;
  std::map<int, AnalogControl> analogs_;
  
  // Timing
  unsigned long current_time_;
  
  // Event processing
  void processKeypadEvent(int event);
  void handleButtonEvent(int button_id, bool pressed);
  void handleEncoderEvent(int col, bool pressed);
};

} // namespace Input

