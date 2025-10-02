#include "features/DiagnosticsMode.h"
#include "hardware/HardwareConfig.h"
#include <Wire.h>

using namespace HardwareConfig;

// Constructor
DiagnosticsMode::DiagnosticsMode(RadioHardware* hardware, EventBus* event_bus)
  : hardware_(hardware),
    event_bus_(event_bus),
    active_(false),
    last_activity_(0),
    monitoring_events_(false),
    monitoring_controls_(false) {
}

// Initialize diagnostics mode
void DiagnosticsMode::begin() {
  Serial.println("Diagnostics mode initialized");
}

// Activate diagnostics mode
void DiagnosticsMode::activate(const char* reason) {
  if (active_) {
    return;  // Already active
  }
  
  active_ = true;
  last_activity_ = millis();
  
  Serial.println();
  Serial.println("=====================================");
  showWelcome(reason);
  Serial.println("=====================================");
  Serial.print("> ");
}

// Deactivate diagnostics mode
void DiagnosticsMode::deactivate() {
  if (!active_) {
    return;
  }
  
  stopMonitoring();
  active_ = false;
  
  Serial.println("\nExiting diagnostics mode...");
  Serial.println("Resuming normal operation");
}

// Show welcome message
void DiagnosticsMode::showWelcome(const char* reason) {
  Serial.println("   DIAGNOSTICS MODE");
  
  if (reason) {
    Serial.println();
    Serial.print("Reason: ");
    Serial.println(reason);
  }
  
  Serial.println();
  Serial.println("Type 'help' for commands");
  Serial.println("Type 'exit' to resume normal operation");
}

// Show help message
void DiagnosticsMode::showHelp() {
  Serial.println("\nAvailable Commands:");
  Serial.println();
  Serial.println("LED Control:");
  Serial.println("  led <sw> <cs> <val>  - Set LED at switch/col (0-255)");
  Serial.println("  led all <val>        - Set all LEDs");
  Serial.println("  led test             - Cycle through all LEDs");
  Serial.println("  led clear            - Clear all LEDs");
  Serial.println();
  Serial.println("Monitoring:");
  Serial.println("  controls             - Monitor button presses (Ctrl+C to stop)");
  Serial.println("  events               - Monitor all events (Ctrl+C to stop)");
  Serial.println();
  Serial.println("Information:");
  Serial.println("  info                 - Show system status");
  Serial.println("  config               - Show configuration");
  Serial.println();
  Serial.println("General:");
  Serial.println("  help or ?            - Show this help");
  Serial.println("  exit or q            - Exit diagnostics mode");
  Serial.println();
}

// Process command from serial input
void DiagnosticsMode::processCommand(const String& command) {
  last_activity_ = millis();
  
  // Trim whitespace
  String cmd = command;
  cmd.trim();
  
  // Empty command
  if (cmd.length() == 0) {
    Serial.print("> ");
    return;
  }
  
  // Parse command - split on first space
  int space_idx = cmd.indexOf(' ');
  String cmd_name = space_idx > 0 ? cmd.substring(0, space_idx) : cmd;
  String args = space_idx > 0 ? cmd.substring(space_idx + 1) : "";
  
  cmd_name.toLowerCase();
  args.trim();
  
  // Dispatch to handlers
  if (cmd_name == "help" || cmd_name == "?") {
    showHelp();
  }
  else if (cmd_name == "exit" || cmd_name == "q") {
    deactivate();
    return;  // Don't print prompt after exit
  }
  else if (cmd_name == "led") {
    handleLEDCommand(args);
  }
  else if (cmd_name == "controls") {
    handleControlsCommand();
  }
  else if (cmd_name == "events") {
    handleEventsCommand();
  }
  else if (cmd_name == "info") {
    handleInfoCommand();
  }
  else if (cmd_name == "config") {
    handleConfigCommand();
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd_name);
    Serial.println("Type 'help' for available commands");
  }
  
  // Print prompt for next command
  Serial.print("> ");
}

// Handle LED control commands
void DiagnosticsMode::handleLEDCommand(const String& args) {
  if (!hardware_) {
    Serial.println("ERROR: Hardware not available");
    return;
  }
  
  // Parse arguments
  int first_space = args.indexOf(' ');
  String subcmd = first_space > 0 ? args.substring(0, first_space) : args;
  subcmd.toLowerCase();
  
  if (subcmd == "clear") {
    // Clear all LEDs
    hardware_->clearAllPresetLEDs();
    hardware_->updatePresetLEDs();
    Serial.println("All LEDs cleared");
  }
  else if (subcmd == "test") {
    // Cycle through all LEDs
    Serial.println("LED test - cycling through all positions...");
    
    for (int row = 0; row < 6; row++) {
      for (int col = 0; col < 16; col++) {
        hardware_->clearAllPresetLEDs();
        hardware_->setLED(row, col, 255);
        hardware_->updatePresetLEDs();
        Serial.printf("  Row=%d Col=%d\n", row, col);
        delay(100);
      }
    }
    
    hardware_->clearAllPresetLEDs();
    hardware_->updatePresetLEDs();
    Serial.println("LED test complete");
  }
  else if (subcmd == "all") {
    // Set all LEDs to brightness
    String rest = first_space > 0 ? args.substring(first_space + 1) : "";
    rest.trim();
    
    int brightness = rest.toInt();
    if (brightness < 0 || brightness > 255) {
      Serial.println("ERROR: Brightness must be 0-255");
      return;
    }
    
    for (int row = 0; row < 6; row++) {
      for (int col = 0; col < 16; col++) {
        hardware_->setLED(row, col, brightness);
      }
    }
    hardware_->updatePresetLEDs();
    Serial.printf("All LEDs set to %d\n", brightness);
  }
  else {
    // Set specific LED: led <row> <col> <brightness>
    int row, col, brightness;
    
    // Parse three integers
    int first = args.indexOf(' ');
    int second = args.indexOf(' ', first + 1);
    
    if (first < 0 || second < 0) {
      Serial.println("ERROR: Usage: led <row> <col> <brightness>");
      Serial.println("              led all <brightness>");
      Serial.println("              led test");
      Serial.println("              led clear");
      return;
    }
    
    row = args.substring(0, first).toInt();
    col = args.substring(first + 1, second).toInt();
    brightness = args.substring(second + 1).toInt();
    
    // Validate ranges
    if (row < 0 || row >= 6) {
      Serial.println("ERROR: Row must be 0-5");
      return;
    }
    if (col < 0 || col >= 16) {
      Serial.println("ERROR: Column must be 0-15");
      return;
    }
    if (brightness < 0 || brightness > 255) {
      Serial.println("ERROR: Brightness must be 0-255");
      return;
    }
    
    // Set LED
    hardware_->setLED(row, col, brightness);
    hardware_->updatePresetLEDs();
    Serial.printf("LED Row=%d Col=%d set to %d\n", row, col, brightness);
  }
}

// Handle controls monitoring
void DiagnosticsMode::handleControlsCommand() {
  if (!event_bus_) {
    Serial.println("ERROR: Event bus not available");
    return;
  }
  
  Serial.println("\n=== CONTROLS MONITOR ===");
  Serial.println("Note: Input controls are now managed by InputManager");
  Serial.println("Use InputManager API to query button/encoder state");
  Serial.println("Press Ctrl+C or any key to return");
  Serial.println();
  
  monitoring_controls_ = true;
  
  // Wait for user to press a key to stop
  while (monitoring_controls_ && active_) {
    if (Serial.available()) {
      Serial.read();  // Consume the character
      break;
    }
    delay(10);
  }
  
  stopMonitoring();
  Serial.println("\n=== MONITORING STOPPED ===");
}

// Handle events monitoring
void DiagnosticsMode::handleEventsCommand() {
  if (!event_bus_) {
    Serial.println("ERROR: Event bus not available");
    return;
  }
  
  Serial.println("\n=== EVENT MONITOR ===");
  Serial.println("Monitoring ALL events");
  Serial.println("Press Ctrl+C or any key to stop");
  Serial.println();
  
  monitoring_events_ = true;
  
  // Subscribe to all event types
  for (uint16_t i = 0; i < static_cast<uint16_t>(EventType::Count); i++) {
    EventType type = static_cast<EventType>(i);
    event_bus_->subscribe(type, eventMonitorCallback, this);
  }
  
  // Wait for user to press a key to stop
  while (monitoring_events_ && active_) {
    if (Serial.available()) {
      Serial.read();  // Consume the character
      break;
    }
    delay(10);
  }
  
  stopMonitoring();
  Serial.println("\n=== MONITORING STOPPED ===");
}

// Handle info command
void DiagnosticsMode::handleInfoCommand() {
  Serial.println("\n=== SYSTEM INFO ===");
  Serial.println();
  
  // Firmware info
  Serial.println("Firmware:");
  Serial.print("  Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.println();
  
  // Hardware status
  Serial.println("Hardware:");
  if (hardware_) {
    Serial.println("  RadioHardware: initialized");
    
    // I2C scan
    Serial.println();
    Serial.println("I2C Devices:");
    Wire.begin();
    for (byte addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      byte error = Wire.endTransmission();
      
      if (error == 0) {
        Serial.print("  0x");
        if (addr < 16) Serial.print("0");
        Serial.print(addr, HEX);
        
        // Identify known devices from config
        if (addr == I2C_ADDR_KEYPAD) Serial.print(" (TCA8418 Keypad)");
        else if (addr == I2C_ADDR_LED_PRESETS) Serial.print(" (IS31FL3737 Preset LEDs)");
        else if (addr == I2C_ADDR_DISPLAY_1) Serial.print(" (IS31FL3737 Display #1)");
        else if (addr == I2C_ADDR_DISPLAY_2) Serial.print(" (IS31FL3737 Display #2)");
        else if (addr == I2C_ADDR_DISPLAY_3) Serial.print(" (IS31FL3737 Display #3)");
        
        Serial.println();
      }
    }
  } else {
    Serial.println("  RadioHardware: NOT initialized");
  }
  
  Serial.println();
  
  // Event bus status
  Serial.println("Event Bus:");
  if (event_bus_) {
    Serial.println("  Status: initialized");
  } else {
    Serial.println("  Status: NOT initialized");
  }
  
  Serial.println();
}

// Handle config command
void DiagnosticsMode::handleConfigCommand() {
  Serial.println("\n=== CONFIGURATION ===");
  Serial.println();
  
  Serial.println("Build Configuration:");
  Serial.println("  ENABLE_DIAGNOSTICS: enabled");
  Serial.print("  CORE_DEBUG_LEVEL: ");
  #ifdef CORE_DEBUG_LEVEL
    Serial.println(CORE_DEBUG_LEVEL);
  #else
    Serial.println("not set");
  #endif
  
  #ifdef BOARD_HAS_PSRAM
    Serial.println("  BOARD_HAS_PSRAM: enabled");
  #endif
  
  Serial.println();
  
  Serial.println("I2C Configuration:");
  Serial.println("  SDA: GPIO21 (ESP32 default)");
  Serial.println("  SCL: GPIO22 (ESP32 default)");
  Serial.println("  Clock: 800 kHz");
  Serial.println();
  
  Serial.println("Hardware Mapping:");
  Serial.printf("  TCA8418 Keypad: 0x%02X\n", I2C_ADDR_KEYPAD);
  Serial.printf("    Row %d: Preset buttons (%d presets)\n", PRESET_BUTTONS[0].row, NUM_PRESETS);
  Serial.printf("    Row %d Col %d-%d: Encoder (A, B, Button)\n", 
                ENCODER_ROW, ENCODER_COL_A, ENCODER_COL_BUTTON);
  Serial.println();
  Serial.println("  IS31FL3737 LED Drivers:");
  Serial.printf("    0x%02X: Preset LEDs\n", I2C_ADDR_LED_PRESETS);
  Serial.printf("    0x%02X: Display #1\n", I2C_ADDR_DISPLAY_1);
  Serial.printf("    0x%02X: Display #2\n", I2C_ADDR_DISPLAY_2);
  Serial.printf("    0x%02X: Display #3\n", I2C_ADDR_DISPLAY_3);
  Serial.println();
}

// Stop monitoring
void DiagnosticsMode::stopMonitoring() {
  if (!event_bus_) {
    return;
  }
  
  if (monitoring_events_) {
    // Unsubscribe from all event types
    for (uint16_t i = 0; i < static_cast<uint16_t>(EventType::Count); i++) {
      EventType type = static_cast<EventType>(i);
      event_bus_->unsubscribe(type, eventMonitorCallback, this);
    }
    monitoring_events_ = false;
  }
  
  if (monitoring_controls_) {
    monitoring_controls_ = false;
  }
}

// Event monitoring callback
void DiagnosticsMode::eventMonitorCallback(const events::Event& evt, void* context) {
  DiagnosticsMode* self = static_cast<DiagnosticsMode*>(context);
  if (!self || !self->monitoring_events_) {
    return;
  }
  
  // Print event with timestamp
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] Event: ");
  Serial.print(eventCatalogLookup(evt.type).name);
  
  // Print payload if present
  if (!evt.value.empty()) {
    Serial.print(" | ");
    Serial.print(evt.value.c_str());
  }
  
  Serial.println();
}

// Control monitoring callback
void DiagnosticsMode::controlMonitorCallback(const events::Event& evt, void* context) {
  DiagnosticsMode* self = static_cast<DiagnosticsMode*>(context);
  if (!self || !self->monitoring_controls_) {
    return;
  }
  
  // Print control event with timestamp
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] ");
  Serial.print(evt.type_name);
  
  // Print payload if present
  if (!evt.value.empty()) {
    Serial.print(" | ");
    Serial.print(evt.value.c_str());
  }
  
  Serial.println();
}
