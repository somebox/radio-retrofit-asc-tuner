#ifndef RADIO_HARDWARE_H
#define RADIO_HARDWARE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TCA8418.h>
#include "IS31FL373x.h"
#include "BrightnessLevels.h"

// Forward declarations
class PresetHandler;
class DisplayManager;

class RadioHardware {
public:
  // Constructor
  RadioHardware();
  
  // Destructor
  ~RadioHardware();
  
  // Initialization
  bool initialize();
  bool verifyHardware();
  void setPresetHandler(PresetHandler* handler);
  
  // Main update function
  void update();
  void scanI2C();

  // Brightness integration
  void setBrightnessLevel(BrightnessLevel level);

  // Progress indication during initialization
  void showProgress(int progress);  // 0-100% progress bar on preset LEDs

  // Helper functions
  bool isValidKeypress(int key_number) const;
  bool isInitialized() const;
  
  // Keypad operations
  bool hasKeypadEvent();
  int getKeypadEvent();
  void configureKeypadMatrix();
  
  // LED operations (high-level interface)
  void setLED(int row, int col, uint8_t brightness);
  void setPresetLED(int preset_num, uint8_t brightness);  // Legacy method
  void clearAllPresetLEDs();
  void updatePresetLEDs();
  void setGlobalBrightness(uint8_t brightness);
  
  // Test functions
  void testPresetLEDs();
  void testKeypadButtons();
  
  // Hardware status
  bool isKeypadReady() const { return keypad_ready_; }
  bool isPresetLEDReady() const { return preset_led_ready_; }
  
private:
  // Hardware instances
  Adafruit_TCA8418 keypad_;
  IS31FL3737* preset_led_driver_;
  
  // Control handlers
  PresetHandler* preset_handler_;
  
  // Status flags
  bool keypad_ready_;
  bool preset_led_ready_;
  bool initialized_;
  
  // Configuration constants
  static const uint8_t KEYPAD_I2C_ADDR = 0x34;  // TCA8418 default address
  static const uint8_t PRESET_LED_I2C_ADDR = 0x55;  // IS31FL3737 with SCL pin
  static const int KEYPAD_ROWS = 4;
  static const int KEYPAD_COLS = 10;
  static const int NUM_PRESETS = 8;  // 7 presets + 1 memory button
  
  // Preset LED mapping (SW0, CS0-CS8 for presets 1-7 + memory)
  struct PresetLEDMapping {
    int sw_pin;
    int cs_pin;
  };
  
  static const PresetLEDMapping PRESET_LED_MAP[NUM_PRESETS];
  
  // Helper methods
  bool initializeKeypad();
  bool initializePresetLEDs();
  bool testI2CDevice(uint8_t address, const char* device_name);
  void mapPresetToLED(int preset_num, int& sw_pin, int& cs_pin);
};

#endif // RADIO_HARDWARE_H
