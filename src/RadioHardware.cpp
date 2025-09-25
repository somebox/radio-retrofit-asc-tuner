#include "RadioHardware.h"
#include "PresetHandler.h"
#include <algorithm>  // For max/min functions
#include "I2CScan.h"

// Preset LED mapping according to RetroText PCB Layout - Column 4 is skipped
const RadioHardware::PresetLEDMapping RadioHardware::PRESET_LED_MAP[NUM_PRESETS] = {
  {0, 0},  // Preset 0: SW1, CS1 (Character 0, top-left)
  {0, 1},  // Preset 1: SW1, CS2 (Character 0, top-center-left)  
  {0, 2},  // Preset 2: SW1, CS3 (Character 0, top-center-right)
  {0, 3},  // Preset 3: SW1, CS5 (Character 1, top-left) - skip CS4
  {0, 5},  // Preset 4: SW1, CS6 (Character 1, top-center-left)
  {0, 6},  // Preset 5: SW1, CS7 (Character 1, top-center-right)
  {0, 7},  // Preset 6: SW1, CS8 (Character 1, top-right)
  {0, 8}   // Memory:   SW1, CS9 (Character 2, top-left)
};

RadioHardware::RadioHardware()
  : preset_led_driver_(nullptr)
  , preset_handler_(nullptr)
  , keypad_ready_(false)
  , preset_led_ready_(false)
  , initialized_(false)
{
}

RadioHardware::~RadioHardware() {
  if (preset_led_driver_) {
    delete preset_led_driver_;
  }
}

bool RadioHardware::initialize() {
  Serial.println("=== RadioHardware Initialization ===");
  
  if (initialized_) {
    Serial.println("RadioHardware already initialized");
    return true;
  }
  
  // Scan I2C bus first
  scanI2C();
  
  // Initialize keypad controller
  Serial.println("Initializing TCA8418 keypad controller...");
  if (!initializeKeypad()) {
    Serial.println("ERROR: Failed to initialize keypad controller");
  }
  
  // Initialize preset LED driver
  Serial.println("Initializing IS31FL3737 preset LED driver...");
  if (!initializePresetLEDs()) {
    Serial.println("ERROR: Failed to initialize preset LED driver");
  }
  
  initialized_ = true;
  
  Serial.println("=== RadioHardware Initialization Complete ===");
  Serial.printf("Keypad ready: %s\n", keypad_ready_ ? "YES" : "NO");
  Serial.printf("Preset LEDs ready: %s\n", preset_led_ready_ ? "YES" : "NO");
  
  return keypad_ready_ || preset_led_ready_;  // Success if at least one component works
}

bool RadioHardware::isInitialized() const {
  return initialized_;
}

void RadioHardware::setPresetHandler(PresetHandler* handler) {
  preset_handler_ = handler;
}

void RadioHardware::update() {
  // Handle keypad events and pass to preset handler
  if (hasKeypadEvent() && preset_handler_) {
    int event = getKeypadEvent();
    bool pressed = event & 0x80;
    int key_number = (event & 0x7F) - 1;
    int row = key_number / KEYPAD_COLS;
    int col = key_number % KEYPAD_COLS;

    // Only handle preset buttons (row 0)
    if (row == 0 && isValidKeypress(key_number)) {
      preset_handler_->handleKeypadEvent(row, col, pressed);
    }
  }
}

bool RadioHardware::verifyHardware() {
  Serial.println("Verifying radio hardware...");
  
  bool all_ok = true;
  
  // Test keypad
  if (keypad_ready_) {
    Serial.println("Testing keypad communication...");
    if (!testI2CDevice(KEYPAD_I2C_ADDR, "TCA8418 Keypad")) {
      all_ok = false;
    }
  }
  
  // Test preset LED driver
  if (preset_led_ready_) {
    Serial.println("Testing preset LED driver communication...");
    if (!testI2CDevice(PRESET_LED_I2C_ADDR, "IS31FL3737 Preset LEDs")) {
      all_ok = false;
    }
  }
  
  return all_ok;
}

void RadioHardware::scanI2C() {
  Serial.println("\nScanning I2C bus for radio hardware...");
  static const I2CKnownDevice known[] = {
    {0x34, "TCA8418 Keypad Controller"},
    {0x55, "IS31FL3737 Preset LEDs (SCL)"},
    {0x50, "IS31FL3737 Display (GND)"},
    {0x5A, "IS31FL3737 Display (VCC)"},
    {0x5F, "IS31FL3737 Display (SDA)"},
  };
  int found = scanI2CBus(known, sizeof(known)/sizeof(known[0]));
  if (found == 0) {
    Serial.println("No I2C devices found via scan");
  } else {
    Serial.printf("Found %d I2C device(s) via scan\n", found);
  }
  Serial.println("I2C scan complete\n");
}

bool RadioHardware::hasKeypadEvent() {
  if (!keypad_ready_) return false;
  
  int available = keypad_.available();
  if (available > 0) {
    Serial.printf("🔍 Keypad event detected: %d events available\n", available);
  }
  return available > 0;
}

int RadioHardware::getKeypadEvent() {
  if (!keypad_ready_) return -1;
  return keypad_.getEvent();
}

void RadioHardware::configureKeypadMatrix() {
  if (!keypad_ready_) {
    Serial.println("WARNING: Keypad not ready, cannot configure matrix");
    return;
  }
  keypad_.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  Serial.printf("Keypad matrix configured for %dx%d (%d buttons)\n", 
                KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ROWS * KEYPAD_COLS);
}

void RadioHardware::setLED(int row, int col, uint8_t brightness) {
  if (!preset_led_ready_) {
    return;
  }
  
  // Direct LED control using row/col (SW/CS pins)
  preset_led_driver_->drawPixel(col, row, brightness);
  
  //Serial.printf("Set LED (SW%d, CS%d) to brightness %d\n", 
  //              row, col, brightness);
}

void RadioHardware::setPresetLED(int preset_num, uint8_t brightness) {
  if (!preset_led_ready_ || preset_num < 1 || preset_num > NUM_PRESETS) {
    return;
  }
  
  int sw_pin, cs_pin;
  mapPresetToLED(preset_num, sw_pin, cs_pin);
  
  // Set the LED brightness
  preset_led_driver_->drawPixel(cs_pin, sw_pin, brightness);
  
  Serial.printf("Set preset %d LED (SW%d, CS%d) to brightness %d\n", 
                preset_num, sw_pin, cs_pin, brightness);
}

void RadioHardware::clearAllPresetLEDs() {
  if (!preset_led_ready_) return;
  
  preset_led_driver_->clear();
  // Serial.println("Cleared all preset LEDs");
}

void RadioHardware::updatePresetLEDs() {
  if (!preset_led_ready_) return;
  
  preset_led_driver_->show();
}

void RadioHardware::setGlobalBrightness(uint8_t brightness) {
  if (!preset_led_ready_) return;
  
  preset_led_driver_->setGlobalCurrent(brightness);
  Serial.printf("Preset LED global brightness set to %d\n", brightness);
}

void RadioHardware::testPresetLEDs() {
  if (!preset_led_ready_) {
    Serial.println("Cannot test preset LEDs - driver not ready");
    return;
  }
  
  Serial.println("Testing preset LEDs...");
  
  // Clear all LEDs first
  clearAllPresetLEDs();
  updatePresetLEDs();
  delay(200);
  
  // Test each preset LED
  for (int preset = 1; preset <= NUM_PRESETS; preset++) {
    Serial.printf("Testing preset %d LED...\n", preset);
    setPresetLED(preset, 255);  // Full brightness
    updatePresetLEDs();
    delay(200);
    
    setPresetLED(preset, 0);    // Turn off
    updatePresetLEDs();
    delay(100);
  }
  
  Serial.println("Preset LED test complete");
}

void RadioHardware::testKeypadButtons() {
  if (!keypad_ready_) {
    Serial.println("Cannot test keypad - controller not ready");
    return;
  }
  
  Serial.println("Testing keypad buttons - press any button for 10 seconds...");
  
  unsigned long test_start = millis();
  int button_presses = 0;
  
  while (millis() - test_start < 10000) {  // 10 second test
    if (hasKeypadEvent()) {
      int event = getKeypadEvent();
      bool pressed = event & 0x80;
      int key_number = (event & 0x7F) - 1;
      
      int row = key_number / KEYPAD_COLS;
      int col = key_number % KEYPAD_COLS;
      
      if (pressed) {
        button_presses++;
        Serial.printf("Button pressed: row=%d, col=%d, key=%d\n", row, col, key_number);
        
        // Light up corresponding preset LED if it's a preset button
        if (row == 0 && col < NUM_PRESETS) {
          setPresetLED(col + 1, 255);  // Preset numbers are 1-based
          updatePresetLEDs();
        }
      } else {
        Serial.printf("Button released: row=%d, col=%d, key=%d\n", row, col, key_number);
        
        // Turn off corresponding preset LED
        if (row == 0 && col < NUM_PRESETS) {
          setPresetLED(col + 1, 0);
          updatePresetLEDs();
        }
      }
    }
    delay(10);
  }
  
  Serial.printf("Keypad test complete - detected %d button presses\n", button_presses);
  
  // Clear all LEDs
  clearAllPresetLEDs();
  updatePresetLEDs();
}

bool RadioHardware::initializeKeypad() {
  Serial.println("Initializing TCA8418 Keypad driver");
  
  // Test I2C communication first
  if (!testI2CDevice(KEYPAD_I2C_ADDR, "TCA8418")) {
    return false;
  }
  
  // Initialize the keypad controller
  if (!keypad_.begin(KEYPAD_I2C_ADDR, &Wire)) {
    Serial.printf("ERROR: TCA8418 initialization failed at address 0x%02X\n", KEYPAD_I2C_ADDR);
    return false;
  }
  
  Serial.printf("keypad driver init at address 0x%02X\n", KEYPAD_I2C_ADDR);
  
  // Configure the keypad matrix AFTER setting keypad_ready_
  keypad_ready_ = true;
  keypad_.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  Serial.printf("Keypad matrix configured for %dx%d (%d total buttons)\n", 
                KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ROWS * KEYPAD_COLS);
  
  Serial.printf("TCA8418 keypad controller initialized successfully at 0x%02X\n", KEYPAD_I2C_ADDR);
  return true;
}

bool RadioHardware::initializePresetLEDs() {
  // Test I2C communication first
  if (!testI2CDevice(PRESET_LED_I2C_ADDR, "IS31FL3737 Preset LEDs")) {
    return false;
  }
  
  // Create the driver instance with SCL address configuration
  preset_led_driver_ = new IS31FL3737(ADDR::SCL);
  
  // Initialize the driver
  if (!preset_led_driver_->begin()) {
    Serial.printf("ERROR: IS31FL3737 preset LED driver initialization failed at address 0x%02X\n", PRESET_LED_I2C_ADDR);
    delete preset_led_driver_;
    preset_led_driver_ = nullptr;
    return false;
  }
  
  // Configure the driver
  preset_led_driver_->setGlobalCurrent(128);  // 50% current
  clearAllPresetLEDs();
  updatePresetLEDs();
  
  preset_led_ready_ = true;
  Serial.printf("IS31FL3737 preset LED driver initialized successfully at 0x%02X\n", PRESET_LED_I2C_ADDR);
  return true;
}

bool RadioHardware::testI2CDevice(uint8_t address, const char* device_name) {
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.printf("✓ %s communication OK at 0x%02X\n", device_name, address);
    return true;
  } else {
    Serial.printf("✗ %s communication failed at 0x%02X (error %d)\n", device_name, address, error);
    return false;
  }
}

void RadioHardware::mapPresetToLED(int preset_num, int& sw_pin, int& cs_pin) {
  if (preset_num >= 1 && preset_num <= NUM_PRESETS) {
    const PresetLEDMapping& mapping = PRESET_LED_MAP[preset_num - 1];  // Convert to 0-based index
    sw_pin = mapping.sw_pin;
    cs_pin = mapping.cs_pin;
  } else {
    sw_pin = 0;
    cs_pin = 0;
  }
}

bool RadioHardware::isValidKeypress(int key_number) const {
  int row = key_number / KEYPAD_COLS;
  int col = key_number % KEYPAD_COLS;
  return (row >= 0 && row < KEYPAD_ROWS && col >= 0 && col < KEYPAD_COLS);
}

void RadioHardware::setBrightnessLevel(BrightnessLevel level) {
  if (preset_led_driver_ && preset_led_ready_) {
    uint8_t brightness = getBrightnessValue(level);
    preset_led_driver_->setGlobalCurrent(brightness);
    Serial.printf("Preset LED brightness level: %d%% (%d)\n", static_cast<int>(level) * 5, brightness);
  } else {
    Serial.printf("WARNING: Cannot set brightness - preset LED driver not ready\n");
  }
}

void RadioHardware::showProgress(int progress) {
  if (!preset_led_driver_ || !preset_led_ready_) return;

  // Clamp progress to 0-100%
  progress = max(0, min(100, progress));

  // Progress from 0-100%, map to 8 LEDs (preset buttons 0-7)
  int active_leds = (progress * 8) / 100;

  // Clear all LEDs first
  clearAllPresetLEDs();

  // Light up LEDs from left to right based on progress
  for (int i = 0; i < 8 && i < active_leds; i++) {
    setLED(0, i, getBrightnessValue(BRIGHTNESS_100_PERCENT));  // Row 0, columns 0-7
  }

  // Update the display immediately
  updatePresetLEDs();

  Serial.printf("Progress bar: %d%% (%d/8 LEDs lit)\n", progress, active_leds);
}
