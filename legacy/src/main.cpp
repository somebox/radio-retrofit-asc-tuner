#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>

// Subsystem headers
#include "display.h"
#include "hardware.h"
#include "platform.h"
#include "features.h"

// Configuration constants
#define NUM_BOARDS 3
#define WIDTH 24
#define HEIGHT 6
#define USER_BUTTON 0

// Timezone config
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Switzerland

// WiFi and time management
WifiTimeLib wifiTimeLib(NTP_SERVER, TZ_INFO);

// Global module instances
DisplayManager* display_manager = nullptr;
ClockDisplay* clock_display = nullptr;
MeteorAnimation* meteor_animation = nullptr;
RadioHardware* radio_hardware = nullptr;
PresetManager* preset_manager = nullptr;
AnnouncementModule* announcement_module = nullptr;
IHomeAssistantBridge* home_assistant_bridge = nullptr;

#ifdef ENABLE_DIAGNOSTICS
  DiagnosticsMode* diagnostics = nullptr;
  DiagnosticsMode* g_diagnostics_instance = nullptr;
#endif

// Mode system
static const char* mode_names[] = {"Retro", "Modern", "Clock", "Animation"};
DisplayMode current_mode = DisplayMode::MODERN;

// Global brightness level management
uint8_t global_brightness = 128;

// SignTextController instances for demonstration
std::unique_ptr<RetroText::SignTextController> modern_sign;
std::unique_ptr<RetroText::SignTextController> retro_sign;

// Forward declarations
void configModeCallback(WiFiManager *myWiFiManager);
void adjustGlobalBrightness(bool increase);
void showBrightnessAnnouncement();
void select_random_message();
void setupWiFi();
void setupModules();

// Message tracking
int current_message_index = 0;
String current_message = "";

// Module announcement messages
const String MODULE_ANNOUNCEMENTS[] = {
  "Retro Font",       // MODE_RETRO
  "Modern Font",      // MODE_MODERN  
  "Clock Display",    // MODE_CLOCK
  "Meteor Animation"  // MODE_ANIMATION
};

// Helper function for backward compatibility (calls DisplayManager)
void display_static_message(String message, bool use_modern_font, int duration_ms) {
  if (display_manager) {
    RetroText::Font font = use_modern_font ? RetroText::MODERN_FONT : RetroText::ARDUBOY_FONT;
    display_manager->displayStaticMessage(message, font, duration_ms);
  }
}

// WiFiManager callback for AP mode
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFi config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  display_static_message("WiFi Config Mode", true, 2000);
}

// Brightness adjustment functions
void adjustGlobalBrightness(bool increase) {
  if (increase) {
    global_brightness = min(255, (int)global_brightness + 30);
  } else {
    global_brightness = max(0, (int)global_brightness - 30);
  }
  
  if (display_manager) {
    display_manager->setBrightnessLevel(global_brightness);
  }
  if (radio_hardware) {
    radio_hardware->setGlobalBrightness(global_brightness);
  }
}

void showBrightnessAnnouncement() {
  if (announcement_module) {
    String brightness_text = "Brightness " + String(global_brightness);
    announcement_module->show(brightness_text, 1000);
  }
}

// Message selection for demo mode
void select_random_message() {
  current_message_index = random(getMessageCount());
  current_message = getMessage(current_message_index);
  Serial.printf("Selected message %d: %s\n", current_message_index, current_message.c_str());
}

// WiFi setup helper
void setupWiFi() {
  Serial.println("Setting up WiFi...");
  display_static_message("WiFi Setup", true, 1000);
  if (radio_hardware) {
    radio_hardware->showProgress(40);
  }

  WiFiManager wm;
  wm.setAPCallback(configModeCallback);

  Serial.println("Attempting WiFi connection...");
  display_static_message("WiFi Connecting", true, 1500);
  if (radio_hardware) {
    radio_hardware->showProgress(60);
  }

  if (wm.autoConnect("RetroText")) {
    Serial.println("WiFi connected, syncing time...");
    display_static_message("WiFi Connected", true, 1000);
    if (radio_hardware) {
      radio_hardware->showProgress(80);
    }

    Serial.println("Syncing time...");
    display_static_message("Syncing Time", true, 500);
    if (radio_hardware) {
      radio_hardware->showProgress(90);
    }

    if (wifiTimeLib.getNTPtime(10, nullptr)) {
      Serial.println("Time synchronized successfully");
      display_static_message("Time Synced", true, 1000);
      if (radio_hardware) {
        radio_hardware->showProgress(100);
        delay(1000);
        radio_hardware->clearAllPresetLEDs();
        radio_hardware->updatePresetLEDs();
      }
    } else {
      Serial.println("Warning: Time sync failed");
      display_static_message("Time Sync Failed", true, 2000);
      if (radio_hardware) {
        radio_hardware->showProgress(95);
        delay(1000);
        radio_hardware->clearAllPresetLEDs();
        radio_hardware->updatePresetLEDs();
      }
    }
  } else {
    Serial.println("Warning: WiFi connection failed");
    display_static_message("WiFi Failed", true, 2000);
    if (radio_hardware) {
      radio_hardware->showProgress(70);
      delay(1000);
      radio_hardware->clearAllPresetLEDs();
      radio_hardware->updatePresetLEDs();
    }
  }
}

// Module initialization helper
void setupModules() {
  // Create feature modules (they handle their own init failures)
  clock_display = new ClockDisplay(display_manager, &wifiTimeLib);
  clock_display->initialize();
  
  meteor_animation = new MeteorAnimation(display_manager);
  meteor_animation->initialize();
  
  home_assistant_bridge = new HomeAssistantBridge();
  home_assistant_bridge->begin();
  
  announcement_module = new AnnouncementModule(display_manager);

  // Create hardware interface
  radio_hardware = new RadioHardware();
  radio_hardware->setEventBus(&eventBus());
  radio_hardware->setBridge(home_assistant_bridge);
  radio_hardware->initialize();  // Handles failures internally
  radio_hardware->setGlobalBrightness(global_brightness);
  
  #ifdef ENABLE_DIAGNOSTICS
    diagnostics = new DiagnosticsMode(radio_hardware, &eventBus());
    g_diagnostics_instance = diagnostics;
    diagnostics->begin();
  #endif

  // Create preset manager (depends on hardware + announcement)
  preset_manager = new PresetManager(radio_hardware, announcement_module);
  preset_manager->initialize();  // Handles failures internally
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Radio Retrofit Firmware ===");
  
  // Initialize I2C bus (critical - fail fast if this doesn't work)
  Wire.begin();  // ESP32 defaults: SDA=GPIO21, SCL=GPIO22
  Wire.setClock(800000);
  Serial.println("I2C: 800kHz, SDA=21, SCL=22");
  
  // Create DisplayManager (hardware may fail gracefully)
  display_manager = new DisplayManager(NUM_BOARDS, WIDTH, HEIGHT);
  display_manager->initialize();
  display_manager->setBrightnessLevel(128);
  
  // Create all modules
  setupModules();
  
  // Initialize message system
  initializeMessages();
  current_message = getMessage(0);
  modern_sign = display_manager->createModernTextController();
  retro_sign = display_manager->createRetroTextController();

  // Self-test and WiFi (optional)
  if (display_manager) {
    display_manager->showTestPattern();
    delay(250);
  }
  
  setupWiFi();
  
  if (display_manager) {
    display_static_message("Ready", true, 500);
  }
  // Don't clear preset LEDs here - PresetManager already initialized them correctly

  pinMode(USER_BUTTON, INPUT_PULLUP);
  Serial.println("Starting demo mode...");
  Serial.println("Press any key for diagnostics mode");
}

void loop() {
  #ifdef ENABLE_DIAGNOSTICS
    // Handle serial input for diagnostics with command history and line editing
    static String input_buffer = "";
    static int cursor_pos = 0;  // Position of cursor in buffer (0 = start, length = end)
    static bool user_exited = false;  // Track if user explicitly exited
    
    while (Serial.available()) {
      char c = Serial.read();
      
      if (c == '\n' || c == '\r') {
        // Process complete line
        Serial.println();  // Echo the newline
        input_buffer.trim();
        
        if (input_buffer.length() > 0 || c == '\n') {
          if (!diagnostics->isActive()) {
            // Only auto-activate if user hasn't explicitly exited
            if (!user_exited) {
              diagnostics->activate();  // First keypress activates
            } else {
              // User exited, ignore input
              input_buffer = "";
              cursor_pos = 0;
              break;
            }
          } else {
            // Process command
            diagnostics->processCommand(input_buffer);
            
            // Check if user exited
            if (!diagnostics->isActive()) {
              user_exited = true;
            }
          }
        }
        
        input_buffer = "";  // Clear buffer
        cursor_pos = 0;
        break;
      } else if (c == 127 || c == 8) {
        // Backspace - delete character before cursor
        if (cursor_pos > 0) {
          // Remove character before cursor
          input_buffer.remove(cursor_pos - 1, 1);
          cursor_pos--;
          
          // Redraw line: move cursor back, print rest of line + space, move cursor to position
          Serial.print("\b");  // Move back one
          String rest = input_buffer.substring(cursor_pos);
          Serial.print(rest);
          Serial.print(" ");  // Clear the deleted character
          // Move cursor back to position
          for (size_t i = 0; i <= rest.length(); i++) {
            Serial.print("\b");
          }
        }
      } else if (c == 0x1B) {
        // ESC key - check for ANSI escape sequence
        delay(1);  // Brief delay to see if this is an escape sequence
        
        if (Serial.available() && Serial.peek() == '[') {
          // ANSI escape sequence - consume the '['
          Serial.read();
          delay(1);
          
          if (Serial.available()) {
            char seq = Serial.read();
            
            if (seq == 'A') {
              // Up arrow - get previous command
              String prev_cmd = diagnostics->getPreviousCommand();
              if (prev_cmd.length() > 0) {
                // Clear current line: move cursor to start, clear to end
                for (int i = 0; i < cursor_pos; i++) {
                  Serial.print("\b");
                }
                for (size_t i = 0; i < input_buffer.length(); i++) {
                  Serial.print(" ");
                }
                for (size_t i = 0; i < input_buffer.length(); i++) {
                  Serial.print("\b");
                }
                // Display previous command
                input_buffer = prev_cmd;
                cursor_pos = input_buffer.length();
                Serial.print(input_buffer);
              }
            } else if (seq == 'B') {
              // Down arrow - get next command
              String next_cmd = diagnostics->getNextCommand();
              // Clear current line
              for (int i = 0; i < cursor_pos; i++) {
                Serial.print("\b");
              }
              for (size_t i = 0; i < input_buffer.length(); i++) {
                Serial.print(" ");
              }
              for (size_t i = 0; i < input_buffer.length(); i++) {
                Serial.print("\b");
              }
              // Display next command (may be empty)
              input_buffer = next_cmd;
              cursor_pos = input_buffer.length();
              Serial.print(input_buffer);
            } else if (seq == 'C') {
              // Right arrow - move cursor right by reprinting the character
              if (cursor_pos < (int)input_buffer.length()) {
                Serial.print(input_buffer[cursor_pos]);
                cursor_pos++;
              }
            } else if (seq == 'D') {
              // Left arrow - move cursor left using backspace
              if (cursor_pos > 0) {
                Serial.print("\b");
                cursor_pos--;
              }
            } else if (seq == 'H') {
              // Home key - move to start
              while (cursor_pos > 0) {
                Serial.print("\b");
                cursor_pos--;
              }
            } else if (seq == 'F') {
              // End key - move to end
              while (cursor_pos < (int)input_buffer.length()) {
                Serial.print(input_buffer[cursor_pos]);
                cursor_pos++;
              }
            }
          }
        } else {
          // Standalone ESC - clear line and reset history position
          for (int i = 0; i < cursor_pos; i++) {
            Serial.print("\b");
          }
          for (size_t i = 0; i < input_buffer.length(); i++) {
            Serial.print(" ");
          }
          for (size_t i = 0; i < input_buffer.length(); i++) {
            Serial.print("\b");
          }
          input_buffer = "";
          cursor_pos = 0;
          diagnostics->resetHistoryPosition();
        }
      } else if (c >= 32 && c < 127) {
        // Printable character - insert at cursor position
        if (input_buffer.length() == 0) {
          diagnostics->resetHistoryPosition();
        }
        
        // Insert character at cursor position
        if (cursor_pos >= (int)input_buffer.length()) {
          // At end of line - simple append
          input_buffer += c;
          Serial.print(c);
          cursor_pos++;
        } else {
          // Insert in middle - need to redraw rest of line
          String before = input_buffer.substring(0, cursor_pos);
          String after = input_buffer.substring(cursor_pos);
          input_buffer = before + c + after;
          
          // Print new character and rest of line
          Serial.print(c);
          Serial.print(after);
          
          // Move cursor back to correct position
          for (size_t i = 0; i < after.length(); i++) {
            Serial.print("\b");
          }
          
          cursor_pos++;
        }
      }
    }
    
    // Skip normal operations when in diagnostics
    if (diagnostics->isActive()) {
      return;
    }
  #endif
  
  static unsigned long last_fps_report = 0;
  static unsigned long frame_count = 0;
  frame_count++;

  // FPS and status report every 5 seconds - suppressed during diagnostics
  #ifdef ENABLE_DIAGNOSTICS
    if (!diagnostics->isActive())
  #endif
  {
    if (millis() - last_fps_report > 5000) {
      float fps = frame_count * 1000.0 / (millis() - last_fps_report);
      Serial.printf("FPS: %.1f | Mode: %s | Announcement: %s\n",
                    fps, mode_names[static_cast<uint8_t>(current_mode)],
                    (announcement_module && announcement_module->isActive()) ? "active" : "idle");
      last_fps_report = millis();
      frame_count = 0;
    }
  }

  // Update hardware and bridge
  if (home_assistant_bridge) {
    home_assistant_bridge->update();
  }
  if (radio_hardware) {
    radio_hardware->update();
    
    // Check for potentiometer changes (controls display and VU meter backlights)
    if (radio_hardware->inputManager().hasAnalog(0)) {
      const Input::AnalogControl& pot = radio_hardware->inputManager().analog(0);
      if (pot.changed()) {
        uint8_t brightness = pot.valueAsByte();
        global_brightness = brightness;
        
        // Update display brightness
        if (display_manager) {
          display_manager->setBrightnessLevel(brightness);
        }
        
        // Update VU meter backlight brightness (not indicator LEDs)
        radio_hardware->setVUMeterBacklightBrightness(brightness);
        
        Serial.printf("Brightness: %d%%\n", pot.valueAsPercent());
      }
    }
  }

  // Check for mode changes
  if (preset_manager && preset_manager->hasModeChanged()) {
    current_mode = preset_manager->getSelectedMode();
    Serial.printf("Mode changed to: %s\n", mode_names[static_cast<uint8_t>(current_mode)]);
    select_random_message();
    
    // Reset scrolling text controllers to start fresh
    if (modern_sign) {
      modern_sign->setMessage(current_message);
      modern_sign->reset();
    }
    if (retro_sign) {
      retro_sign->setMessage(current_message);
      retro_sign->reset();
    }
    
    preset_manager->clearModeChanged();
  }

  // Update modules
  if (preset_manager) {
    preset_manager->update();
  }
  if (announcement_module) {
    announcement_module->update();
  }

  // Handle different display modes
  if (!announcement_module || !announcement_module->isActive()) {
    switch (current_mode) {
      case DisplayMode::RETRO:
        if (retro_sign) {
          // Only set message if it changed, don't set every frame (prevents scrolling)
          if (retro_sign->getMessage() != current_message) {
            retro_sign->setMessage(current_message);
          }
          retro_sign->update();
        }
        break;
        
      case DisplayMode::MODERN:
        if (modern_sign) {
          // Only set message if it changed, don't set every frame (prevents scrolling)
          if (modern_sign->getMessage() != current_message) {
            modern_sign->setMessage(current_message);
          }
          modern_sign->update();
        }
        break;
        
      case DisplayMode::CLOCK:
        if (clock_display) {
          clock_display->update();
        }
        break;
        
      case DisplayMode::ANIMATION:
        if (meteor_animation) {
          meteor_animation->update();
        }
        break;
    }
  }
  
  delay(10);
}
