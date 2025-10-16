#include "hardware/RadioHardware.h"
#include "hardware/HardwareConfig.h"
#include "platform/HomeAssistantBridge.h"
#include "hardware/PresetManager.h"
#include <algorithm>  // For max/min functions
#include <string>
#include "platform/I2CScan.h"
#include "platform/JsonHelpers.h"

using namespace HardwareConfig;

namespace {
class RadioBridgeHandlerImpl : public IHomeAssistantCommandHandler {
 public:
  explicit RadioBridgeHandlerImpl(RadioHardware* hardware) : hardware_(hardware) {}

  void onSetMode(int mode, const String& mode_name, int preset) override {
    hardware_->handleBridgeSetMode(mode, mode_name.c_str(), preset);
  }

  void onSetVolume(int volume) override {
    hardware_->handleBridgeSetVolume(volume);
  }

  void onSetBrightness(int value) override {
    hardware_->handleBridgeSetBrightness(value);
  }

  void onSetMetadata(const String& text) override {
    hardware_->handleBridgeSetMetadata(text.c_str());
  }

  void onRequestStatus() override {
    hardware_->handleBridgeStatusRequest();
  }

 private:
  RadioHardware* hardware_;
};
}

static RadioBridgeHandlerImpl bridge_handler(nullptr);

RadioHardware::RadioHardware()
  : preset_led_driver_(nullptr)
  , event_bus_(nullptr)
  , bridge_(nullptr)
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
  
  // Setup InputManager
  if (keypad_ready_) {
    Serial.println("Initializing InputManager...");
    input_manager_.setKeypad(&keypad_);
    
    // Register preset buttons (0-7)
    for (int i = 0; i < NUM_PRESETS; i++) {
      input_manager_.registerButton(i);
    }
    
    // Register encoder (ID 0)
    input_manager_.registerEncoder(0);
    
    Serial.println("InputManager initialized with 8 buttons + encoder");
  }
  
  // Register potentiometer (ID 0, always available)
  // Deadzone: 50 (out of 4095), Min update interval: 150ms (prevents oscillations)
  input_manager_.registerAnalog(0, HardwareConfig::PIN_VOLUME_POT, 50, 150);
  
  // Initialize VU meter backlights to dim level
  setVUMeterBacklightBrightness(HardwareConfig::LED_BRIGHTNESS_DIM);
  
  initialized_ = true;
  
  Serial.println("=== RadioHardware Initialization Complete ===");
  Serial.printf("Keypad ready: %s\n", keypad_ready_ ? "YES" : "NO");
  Serial.printf("Preset LEDs ready: %s\n", preset_led_ready_ ? "YES" : "NO");
  
  return keypad_ready_ || preset_led_ready_;  // Success if at least one component works
}

bool RadioHardware::isInitialized() const {
  return initialized_;
}

void RadioHardware::setEventBus(EventBus* bus) {
  event_bus_ = bus;
}

void RadioHardware::setBridge(IHomeAssistantBridge* bridge) {
  bridge_ = bridge;
  if (bridge_) {
    bridge_handler = RadioBridgeHandlerImpl(this);
    bridge_->setHandler(&bridge_handler);
  }
}

void RadioHardware::update() {
  // Update InputManager (polls keypad and updates control states)
  input_manager_.update();
  
  // Note: Encoder is for local navigation only, not published to ESPHome
  // Device state changes (mode, brightness, volume) are published instead
}

bool RadioHardware::verifyHardware() {
  Serial.println("Verifying radio hardware...");
  
  bool all_ok = true;
  
  // Test keypad
  if (keypad_ready_) {
    Serial.println("Testing keypad communication...");
    if (!testI2CDevice(I2C_ADDR_KEYPAD, "TCA8418 Keypad")) {
      all_ok = false;
    }
  }
  
  // Test preset LED driver
  if (preset_led_ready_) {
    Serial.println("Testing preset LED driver communication...");
    if (!testI2CDevice(I2C_ADDR_LED_PRESETS, "IS31FL3737 Preset LEDs")) {
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

// configureKeypadMatrix removed - configuration handled in initializeKeypad()

void RadioHardware::setLED(int row, int col, uint8_t brightness) {
  if (!preset_led_ready_) {
    return;
  }
  
  // Direct LED control using row/col (SW/CS pins)
  // IS31FL3737 drawPixel takes (x=CS, y=SW, brightness)
  preset_led_driver_->drawPixel(col, row, brightness);
}

void RadioHardware::setPresetLED(int preset_num, uint8_t brightness) {
  // Convert to 0-based index
  int preset_idx = preset_num - 1;
  
  if (!preset_led_ready_ || preset_idx < 0 || preset_idx >= NUM_PRESETS) {
    return;
  }
  
  // Get LED mapping from config
  const PresetLED* led = getPresetLED(preset_idx);
  if (!led) return;
  
  // Set the LED brightness
  preset_led_driver_->drawPixel(led->cs_pin, led->sw_pin, brightness);
  
  Serial.printf("Set preset %d LED (SW%d, CS%d) to brightness %d\n", 
                preset_num, led->sw_pin, led->cs_pin, brightness);
}

void RadioHardware::clearAllPresetLEDs() {
  if (!preset_led_ready_) return;
  
  // Only clear preset button LEDs (row 3, cols 0-8), not the entire matrix
  // This preserves mode LEDs, VU meter LEDs, etc.
  using namespace HardwareConfig;
  for (int i = 0; i < NUM_PRESETS; i++) {
    const auto* led = getPresetLED(i);
    if (led) {
      preset_led_driver_->drawPixel(led->cs_pin, led->sw_pin, 0);
    }
  }
}

void RadioHardware::updatePresetLEDs() {
  if (!preset_led_ready_) return;
  
  preset_led_driver_->show();
}

void RadioHardware::setGlobalBrightness(uint8_t brightness) {
  if (!preset_led_ready_) return;
  
  preset_led_driver_->setGlobalCurrent(brightness);
}

void RadioHardware::setVUMeterBacklightBrightness(uint8_t brightness) {
  if (!preset_led_ready_) return;
  
  // Set only the VU meter backlight LEDs (indices 2 and 4)
  // Index 2: Tuning Backlight (row 3, col 9)
  // Index 4: Signal Backlight (row 3, col 10)
  const auto* tuning_backlight = HardwareConfig::getVUMeterLED(2);
  const auto* signal_backlight = HardwareConfig::getVUMeterLED(4);
  
  if (tuning_backlight) {
    setLED(tuning_backlight->sw_pin, tuning_backlight->cs_pin, brightness);
  }
  if (signal_backlight) {
    setLED(signal_backlight->sw_pin, signal_backlight->cs_pin, brightness);
  }
  
  preset_led_driver_->show();
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
  }
  
  Serial.printf("Keypad test complete - detected %d button presses\n", button_presses);
  
  // Clear all LEDs
  clearAllPresetLEDs();
  updatePresetLEDs();
}

bool RadioHardware::initializeKeypad() {
  Serial.println("Initializing TCA8418 Keypad driver");
  
  // Test I2C communication first
  if (!testI2CDevice(I2C_ADDR_KEYPAD, "TCA8418")) {
    return false;
  }
  
  // Initialize the keypad controller
  if (!keypad_.begin(I2C_ADDR_KEYPAD, &Wire)) {
    Serial.printf("ERROR: TCA8418 initialization failed at address 0x%02X\n", I2C_ADDR_KEYPAD);
    return false;
  }
  
  Serial.printf("keypad driver init at address 0x%02X\n", I2C_ADDR_KEYPAD);
  
  // Configure the keypad matrix AFTER setting keypad_ready_
  keypad_ready_ = true;
  keypad_.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  Serial.printf("Keypad matrix configured for %dx%d (%d total buttons)\n", 
                KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ROWS * KEYPAD_COLS);
  
  // Flush all events (key + GPIO) and clear INT_STAT register
  // This clears any stale events from hardware power-on or previous session
  uint8_t flushed = keypad_.flush();
  if (flushed > 0) {
    Serial.printf("Flushed %d stale events from TCA8418 FIFO\n", flushed);
  }
  
  Serial.printf("TCA8418 keypad controller initialized successfully at 0x%02X\n", I2C_ADDR_KEYPAD);
  return true;
}

bool RadioHardware::initializePresetLEDs() {
  // Test I2C communication first
  if (!testI2CDevice(I2C_ADDR_LED_PRESETS, "IS31FL3737 Preset LEDs")) {
    return false;
  }
  
  // Create the driver instance with SCL address configuration
  preset_led_driver_ = new IS31FL3737(ADDR::SCL);
  
  // Initialize the driver
  if (!preset_led_driver_->begin()) {
    Serial.printf("ERROR: IS31FL3737 preset LED driver initialization failed at address 0x%02X\n", I2C_ADDR_LED_PRESETS);
    delete preset_led_driver_;
    preset_led_driver_ = nullptr;
    return false;
  }
  
  // Configure the driver
  preset_led_driver_->setGlobalCurrent(128);  // 50% current
  clearAllPresetLEDs();
  updatePresetLEDs();
  
  preset_led_ready_ = true;
  Serial.printf("IS31FL3737 preset LED driver initialized successfully at 0x%02X\n", I2C_ADDR_LED_PRESETS);
  return true;
}

// ADC and GPIO initialization removed - not yet implemented

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

// isValidKeypress removed - InputManager handles all button validation

void RadioHardware::showProgress(int progress) {
  if (!preset_led_driver_ || !preset_led_ready_) return;

  // Clamp progress to 0-100%
  progress = max(0, min(100, progress));

  // Progress from 0-100%, map to 8 preset LEDs
  int active_leds = (progress * 8) / 100;

  // Clear all preset LEDs first (only touches preset LEDs, not mode LEDs)
  clearAllPresetLEDs();

  // Light up preset LEDs from left to right based on progress using HardwareConfig
  // Physical order: Preset 1, 2, 3, 4, 5, 6, 7, Memory (left to right on panel)
  // Array order: Index 0=Preset 1, 1=Preset 2, etc.
  using namespace HardwareConfig;
  for (int i = 0; i < 8 && i < active_leds && i < NUM_PRESETS; i++) {
    const auto* led = getPresetLED(i);
    if (led) {
      setLED(led->sw_pin, led->cs_pin, 255);
      Serial.printf("  Progress LED %d: SW%d CS%d\n", i+1, led->sw_pin, led->cs_pin);
    }
  }

  // Update the display immediately
  updatePresetLEDs();

  Serial.printf("Progress bar: %d%% (%d/8 LEDs lit)\n", progress, active_leds);
}


// handlePresetKeyEvent removed - PresetManager handles button logic directly via InputManager

void RadioHardware::handleBridgeSetMode(int mode, const char* mode_name, int preset) {
  if (!event_bus_) return;
  events::Event evt(EventType::ModeChanged);
  evt.timestamp = millis();
  evt.value = events::json::object({
      events::json::number_field("value", mode),
      events::json::string_field("name", mode_name, mode_name && mode_name[0]),
      events::json::number_field("preset", preset, preset >= 0),
  });
  event_bus_->publish(evt);
}

void RadioHardware::handleBridgeSetVolume(int volume) {
  events::Event evt(EventType::VolumeChanged);
  evt.timestamp = millis();
  evt.value = events::json::object({
      events::json::number_field("value", volume),
  });
  if (event_bus_) {
    event_bus_->publish(evt);
  }
}

void RadioHardware::handleBridgeSetBrightness(int value) {
  setGlobalBrightness(static_cast<uint8_t>(value));
}

void RadioHardware::handleBridgeSetMetadata(const char* text) {
  events::Event evt(EventType::AnnouncementRequested);
  evt.timestamp = millis();
  evt.value = events::json::object({
      events::json::string_field("text", text, text && text[0]),
  });
  if (event_bus_) {
    event_bus_->publish(evt);
  }
}

void RadioHardware::handleBridgeStatusRequest() {
  if (!bridge_) return;
  events::Event evt(EventType::ModeChanged);
  evt.timestamp = millis();
  evt.value = events::json::object({
      events::json::number_field("value", 0),
  });
  bridge_->publishEvent(evt);
}

// handleEncoderEvent removed - encoder logic now handled by InputManager

// Encoder event publishing removed - encoder is for local navigation only
// Device state changes (mode, brightness, preset) are published instead
