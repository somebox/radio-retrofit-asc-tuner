#include "features/DiagnosticsMode.h"
#include "hardware/HardwareConfig.h"
#include <Wire.h>

using namespace HardwareConfig;

// IS31FL3737 is a 12x12 LED matrix driver
constexpr int LED_DRIVER_ROWS = HardwareConfig::LED_MATRIX_ROWS;
constexpr int LED_DRIVER_COLS = HardwareConfig::LED_MATRIX_COLS;

// Constructor
DiagnosticsMode::DiagnosticsMode(RadioHardware* hardware, EventBus* event_bus)
  : hardware_(hardware),
    event_bus_(event_bus),
    active_(false),
    last_activity_(0),
    monitoring_events_(false),
    monitoring_controls_(false),
    history_position_(-1) {
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
  Serial.println("  controls             - Monitor raw TCA8418 keypad (any key to stop)");
  Serial.println("  events               - Monitor all events (any key to stop)");
  Serial.println();
  Serial.println("Information:");
  Serial.println("  info                 - Show system status");
  Serial.println("  config               - Show configuration");
  Serial.println();
  Serial.println("General:");
  Serial.println("  help or ?            - Show this help");
  Serial.println("  exit or q            - Exit diagnostics mode");
  Serial.println();
  Serial.println("Navigation & Editing:");
  Serial.println("  Up/Down arrows       - Navigate command history");
  Serial.println("  Left/Right arrows    - Move cursor within line");
  Serial.println("  Home/End             - Jump to start/end of line");
  Serial.println("  Backspace            - Delete character before cursor");
  Serial.println("  ESC                  - Clear current line");
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
  
  // Add to command history (avoid duplicates of the last command)
  if (command_history_.empty() || command_history_.back() != cmd) {
    command_history_.push_back(cmd);
    
    // Limit history size
    if (command_history_.size() > MAX_HISTORY) {
      command_history_.erase(command_history_.begin());
    }
  }
  
  // Reset history position after executing a command
  history_position_ = -1;
  
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
    Serial.printf("Testing %dx%d matrix (SW1-SW%d, CS1-CS%d)\n", 
                  LED_DRIVER_ROWS, LED_DRIVER_COLS, 
                  LED_DRIVER_ROWS, LED_DRIVER_COLS);
    
    for (int row = 0; row < LED_DRIVER_ROWS; row++) {
      for (int col = 0; col < LED_DRIVER_COLS; col++) {
        hardware_->clearAllPresetLEDs();
        hardware_->setLED(row, col, 255);
        hardware_->updatePresetLEDs();
        Serial.printf("  SW%d CS%d\n", row+1, col+1);  // Show 1-based labels
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
    
    for (int row = 0; row < LED_DRIVER_ROWS; row++) {
      for (int col = 0; col < LED_DRIVER_COLS; col++) {
        hardware_->setLED(row, col, brightness);
      }
    }
    hardware_->updatePresetLEDs();
    Serial.printf("All LEDs set to %d (%dx%d matrix = %d LEDs)\n", 
                  brightness, LED_DRIVER_ROWS, LED_DRIVER_COLS, 
                  LED_DRIVER_ROWS * LED_DRIVER_COLS);
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
    if (row < 0 || row >= LED_DRIVER_ROWS) {
      Serial.printf("ERROR: Row must be 0-%d (SW1-SW%d on board)\n", LED_DRIVER_ROWS-1, LED_DRIVER_ROWS);
      return;
    }
    if (col < 0 || col >= LED_DRIVER_COLS) {
      Serial.printf("ERROR: Column must be 0-%d (CS1-CS%d on board)\n", LED_DRIVER_COLS-1, LED_DRIVER_COLS);
      return;
    }
    if (brightness < 0 || brightness > 255) {
      Serial.println("ERROR: Brightness must be 0-255");
      return;
    }
    
    // Set LED
    hardware_->setLED(row, col, brightness);
    hardware_->updatePresetLEDs();
    Serial.printf("LED SW%d CS%d set to %d\n", row+1, col+1, brightness);  // Show 1-based labels
  }
}

// Handle controls monitoring
void DiagnosticsMode::handleControlsCommand() {
  if (!hardware_) {
    Serial.println("ERROR: Hardware not available");
    return;
  }
  
  Serial.println("\n=== RAW KEYPAD MONITOR ===");
  Serial.println("Monitoring TCA8418 keypad events (raw row/col)");
  Serial.println("Press buttons/encoder to see their row/col position");
  Serial.println("Press any serial key to stop monitoring");
  Serial.println();
  
  // Flush serial buffer to avoid immediate exit
  delay(100);
  while (Serial.available()) {
    Serial.read();
  }
  
  monitoring_controls_ = true;
  unsigned long last_check = 0;
  
  // Monitor for keypad events
  while (monitoring_controls_ && active_) {
    // Check for serial input to exit
    if (Serial.available()) {
      Serial.read();  // Consume the character
      break;
    }
    
    // Check for keypad events (only check every 10ms to avoid spam)
    if (millis() - last_check >= 10) {
      last_check = millis();
      
      if (hardware_->hasKeypadEvent()) {
        int evt = hardware_->getKeypadEvent();
        if (evt >= 0) {
          // Decode TCA8418 event: lower 7 bits = key code, bit 7 = press(0)/release(1)
          bool is_release = (evt & 0x80) != 0;
          int key_code = evt & 0x7F;
          
          // Calculate row and column from key code
          // TCA8418 uses: key_code = (row * 10) + col
          int row = key_code / 10;
          int col = key_code % 10;
          
          Serial.printf("[%lu] %s: Row=%d Col=%d (keycode=%d, raw=0x%02X)\n",
                       millis(),
                       is_release ? "RELEASE" : "PRESS  ",
                       row, col, key_code, evt);
        }
      }
    }
    
    delay(1);  // Small delay to avoid busy-waiting
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
  Serial.println("Press any key to stop monitoring");
  Serial.println();
  
  // Flush serial buffer to avoid immediate exit
  delay(100);
  while (Serial.available()) {
    Serial.read();
  }
  
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

// Get previous command from history (up arrow)
String DiagnosticsMode::getPreviousCommand() {
  if (command_history_.empty()) {
    return "";
  }
  
  // Initialize position to end if not navigating
  if (history_position_ == -1) {
    history_position_ = command_history_.size();
  }
  
  // Move backward in history
  if (history_position_ > 0) {
    history_position_--;
    return command_history_[history_position_];
  }
  
  // Already at oldest command
  return command_history_[0];
}

// Get next command from history (down arrow)
String DiagnosticsMode::getNextCommand() {
  if (command_history_.empty() || history_position_ == -1) {
    return "";
  }
  
  // Move forward in history
  if (history_position_ < (int)command_history_.size() - 1) {
    history_position_++;
    return command_history_[history_position_];
  }
  
  // At end of history, return to empty line
  history_position_ = -1;
  return "";
}

// Reset history navigation to most recent
void DiagnosticsMode::resetHistoryPosition() {
  history_position_ = -1;
}
