#ifndef RADIO_HARDWARE_H
#define RADIO_HARDWARE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TCA8418.h>
#include "IS31FL373x.h"
#include "platform/events/Events.h"
#include "platform/InputManager.h"

// Forward declarations
class IHomeAssistantBridge;
class IHomeAssistantCommandHandler;

// Forward declarations
class RadioHardware {
public:
  // Constructor
  RadioHardware();
  
  // Destructor
  ~RadioHardware();
  
  // Initialization
  bool initialize();
  bool verifyHardware();
  void setEventBus(EventBus* bus);
  void setBridge(IHomeAssistantBridge* bridge);
  
  // Main update function
  void update();
  void scanI2C();
  void handleBridgeSetMode(int mode, const char* mode_name, int preset);
  void handleBridgeSetVolume(int volume);
  void handleBridgeSetBrightness(int value);
  void handleBridgeSetMetadata(const char* text);
  void handleBridgeStatusRequest();

  // Progress indication during initialization
  void showProgress(int progress);  // 0-100% progress bar on preset LEDs

  // Helper functions
  bool isInitialized() const;
  
  // Keypad status (deprecated - use InputManager)
  bool hasKeypadEvent();
  int getKeypadEvent();
  
  // LED operations (high-level interface)
  void setLED(int row, int col, uint8_t brightness);
  void setPresetLED(int preset_num, uint8_t brightness);  // Legacy method
  void clearAllPresetLEDs();
  void updatePresetLEDs();
  void setGlobalBrightness(uint8_t brightness);
  void setVUMeterBacklightBrightness(uint8_t brightness);  // VU meter backlights only
  
  // Test functions
  void testPresetLEDs();
  void testKeypadButtons();
  
  // Hardware status
  bool isKeypadReady() const { return keypad_ready_; }
  bool isPresetLEDReady() const { return preset_led_ready_; }
  
  // Input management
  Input::InputManager& inputManager() { return input_manager_; }
  const Input::InputManager& inputManager() const { return input_manager_; }
  
private:
  friend class RadioBridgeHandlerImpl;

  // Hardware instances
  Adafruit_TCA8418 keypad_;
  IS31FL3737* preset_led_driver_;
  Input::InputManager input_manager_;
  
  // Control handlers
  EventBus* event_bus_;
  IHomeAssistantBridge* bridge_;
  
  // Status flags
  bool keypad_ready_;
  bool preset_led_ready_;
  bool initialized_;
  
  // Configuration is centralized in HardwareConfig.h
  // No local configuration constants - see HardwareConfig.h for all hardware settings
  
  // Helper methods
  bool initializeKeypad();
  bool initializePresetLEDs();
  bool testI2CDevice(uint8_t address, const char* device_name);
};

#endif // RADIO_HARDWARE_H
