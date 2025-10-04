#pragma once

/**
 * @file HardwareConfig.h
 * @brief Hardware Pin Assignments & I2C Addresses
 * 
 * Defines how hardware components are physically connected.
 * Modify this file when changing wiring.
 */

namespace HardwareConfig {

// ============================================================================
// I2C BUS
// ============================================================================

// I2C Device Addresses
constexpr uint8_t I2C_ADDR_KEYPAD       = 0x34;  // TCA8418 Keypad Controller
constexpr uint8_t I2C_ADDR_LED_PRESETS  = 0x55;  // IS31FL3737 Preset LEDs (ADDR=SCL)
constexpr uint8_t I2C_ADDR_DISPLAY_1    = 0x50;  // IS31FL3737 Display #1 (ADDR=GND)
constexpr uint8_t I2C_ADDR_DISPLAY_2    = 0x5A;  // IS31FL3737 Display #2 (ADDR=VCC)
constexpr uint8_t I2C_ADDR_DISPLAY_3    = 0x5F;  // IS31FL3737 Display #3 (ADDR=SDA)

// ============================================================================
// TCA8418 KEYPAD MATRIX
// ============================================================================

constexpr int KEYPAD_ROWS = 4;
constexpr int KEYPAD_COLS = 10;

// Preset Buttons (Row 0)
struct PresetButton {
  const char* name;
  int row;
  int col;
};

constexpr int PRESET_BUTTON_ROW        = 3;
constexpr PresetButton PRESET_BUTTONS[] = {
  {"Preset 1", PRESET_BUTTON_ROW, 3},
  {"Preset 2", PRESET_BUTTON_ROW, 2},
  {"Preset 3", PRESET_BUTTON_ROW, 1},
  {"Preset 4", PRESET_BUTTON_ROW, 0},
  {"Preset 5", PRESET_BUTTON_ROW, 8}, 
  {"Preset 6", PRESET_BUTTON_ROW, 7},
  {"Preset 7", PRESET_BUTTON_ROW, 6},
  {"Memory",   PRESET_BUTTON_ROW, 5}
};

constexpr int NUM_PRESETS = sizeof(PRESET_BUTTONS) / sizeof(PRESET_BUTTONS[0]);

// Rotary Encoder (Row 1)
constexpr int ENCODER_ROW        = 2;
constexpr int ENCODER_COL_A      = 3;  // Channel A
constexpr int ENCODER_COL_B      = 4;  // Channel B
constexpr int ENCODER_COL_BUTTON = 2;  // Push button

// Mode Selector Switch (Row 2) - 4-position switch
constexpr int MODE_SELECTOR_ROW = 2;
struct ModeSelectorButton {
  const char* name;
  int col;
};

constexpr ModeSelectorButton MODE_SELECTOR_BUTTONS[] = {
  {"Stereo",      0},  // Position 1
  {"Stereo-Far",  1},  // Position 2
  {"Q",           2},  // Position 3
  {"Mono",        3}   // Position 4
};

constexpr int NUM_MODE_POSITIONS = sizeof(MODE_SELECTOR_BUTTONS) / sizeof(MODE_SELECTOR_BUTTONS[0]);

// ============================================================================
// IS31FL3737 PRESET LED MATRIX
// ============================================================================

// IS31FL3737 matrix dimensions (12x12 driver)
constexpr int LED_MATRIX_ROWS = 12;  // SW0-SW11 (0-based)
constexpr int LED_MATRIX_COLS = 12;  // CS0-CS11 (0-based)

// LED positions (SW=row, CS=column on LED driver)
struct PresetLED {
  int sw_pin;
  int cs_pin;
};

constexpr PresetLED PRESET_LEDS[] = {
  {3, 3},  // Preset 1
  {3, 2},  // Preset 2
  {3, 1},  // Preset 3
  {3, 0},  // Preset 4
  {3, 8},  // Preset 5 (CS4 skipped)
  {3, 7},  // Preset 6
  {3, 6},  // Preset 7
  {3, 5}   // Memory
};

static_assert(sizeof(PRESET_LEDS) / sizeof(PRESET_LEDS[0]) == NUM_PRESETS, 
              "PresetLED count must match PresetButton count");

// Mode Indicator LEDs (Row 2) - 4 LEDs showing active mode
constexpr int MODE_LED_ROW = 0;
struct ModeLED {
  const char* name;
  int sw_pin;
  int cs_pin;
};

constexpr ModeLED MODE_LEDS[] = {
  {"Stereo",     MODE_LED_ROW, 7},
  {"Stereo-Far", MODE_LED_ROW, 6},
  {"Q",          MODE_LED_ROW, 8},
  {"Mono",       MODE_LED_ROW, 5}
};

constexpr int NUM_MODE_LEDS = sizeof(MODE_LEDS) / sizeof(MODE_LEDS[0]);

// VU Meter LEDs (Row 2) - 5 LEDs total for 2 analog meters
constexpr int VU_METER_ROW = 2;
struct VUMeterLED {
  const char* name;
  int sw_pin;
  int cs_pin;
};

constexpr VUMeterLED VU_METER_LEDS[] = {
  {"Tuning Bar 1",      VU_METER_ROW, 0},  // First meter channel 1
  {"Tuning Bar 2",      VU_METER_ROW, 1},  // First meter channel 2
  {"Tuning Backlight",  VU_METER_ROW, 9},  // First meter illumination
  {"Signal Bar",        VU_METER_ROW, 3},  // Second meter channel
  {"Signal Backlight",  VU_METER_ROW, 10}   // Second meter illumination
};

constexpr int NUM_VU_METER_LEDS = sizeof(VU_METER_LEDS) / sizeof(VU_METER_LEDS[0]);

// ============================================================================
// ESP32 ANALOG INPUT (ADC)
// ============================================================================

// Volume/Muting potentiometer on AS5000E front panel
// Connected to ADC1_CH5, reads 0-4095 (12-bit)
// Used for screen brightness control with noise reduction (deadzone + time throttling)
constexpr int PIN_VOLUME_POT = 33;  // GPIO33 (ADC1_CH5)

// ============================================================================
// LED BRIGHTNESS LEVELS
// ============================================================================

constexpr uint8_t LED_BRIGHTNESS_OFF    = 0;
constexpr uint8_t LED_BRIGHTNESS_DIM    = 64;
constexpr uint8_t LED_BRIGHTNESS_FULL   = 255;

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================

constexpr int DISPLAY_NUM_BOARDS = 3;
constexpr int DISPLAY_WIDTH      = 24;  // Total width in columns
constexpr int DISPLAY_HEIGHT     = 6;   // Total height in rows

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline const PresetButton* getPresetButton(int index) {
  return (index >= 0 && index < NUM_PRESETS) ? &PRESET_BUTTONS[index] : nullptr;
}

inline const PresetLED* getPresetLED(int index) {
  return (index >= 0 && index < NUM_PRESETS) ? &PRESET_LEDS[index] : nullptr;
}

inline int findPresetByButton(int row, int col) {
  for (int i = 0; i < NUM_PRESETS; i++) {
    if (PRESET_BUTTONS[i].row == row && PRESET_BUTTONS[i].col == col) {
      return i;
    }
  }
  return -1;
}

inline bool isEncoderButton(int row, int col) {
  return (row == ENCODER_ROW && 
          (col == ENCODER_COL_A || col == ENCODER_COL_B || col == ENCODER_COL_BUTTON));
}

inline int findModeSelectorPosition(int col) {
  for (int i = 0; i < NUM_MODE_POSITIONS; i++) {
    if (MODE_SELECTOR_BUTTONS[i].col == col) {
      return i;
    }
  }
  return -1;
}

inline const ModeLED* getModeLED(int index) {
  return (index >= 0 && index < NUM_MODE_LEDS) ? &MODE_LEDS[index] : nullptr;
}

inline const VUMeterLED* getVUMeterLED(int index) {
  return (index >= 0 && index < NUM_VU_METER_LEDS) ? &VU_METER_LEDS[index] : nullptr;
}

} // namespace HardwareConfig
