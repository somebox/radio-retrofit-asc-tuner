#include <Arduino.h>
#include <Wire.h>
#include <fonts/retro_font4x6.h>
#include <fonts/modern_font4x6.h>  // Include the converted alternative font
#include <sstream> // used for parsing and building strings
#include <iostream>
#include <string>
#include "IS31FL373x.h"
#include "WifiTimeLib.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include "SignTextController.h"
#include "I2CScan.h"
#include "messages.h"
#include "DisplayManager.h"
#include "ClockDisplay.h"
#include "MeteorAnimation.h"
#include "RadioHardware.h"
#include "PresetHandler.h"
#include "AnnouncementModule.h"
#include "BrightnessLevels.h"

// Alternative font system
#define FONT_WIDTH 4
#define FONT_HEIGHT 6  // Back to 6 rows with adaptive character shifting

// Brightness levels are now managed by DisplayManager
// Use DisplayManager::BrightnessLevel enum values instead

#include "DisplayMode.h"

// Mode system - centralized in DisplayMode.h
static const char* mode_names[] = {"Retro", "Modern", "Clock", "Animation"};

DisplayMode current_mode = DisplayMode::MODERN;  // Start with Modern mode (preset 0)




// Timezone config
/* 
  Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  based on https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
*/
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Switzerland

// WiFi and time management
WifiTimeLib wifiTimeLib(NTP_SERVER, TZ_INFO);

// Global module instances
DisplayManager* display_manager = nullptr;
ClockDisplay* clock_display = nullptr;
MeteorAnimation* meteor_animation = nullptr;
RadioHardware* radio_hardware = nullptr;
PresetHandler* preset_handler = nullptr;
AnnouncementModule* announcement_module = nullptr;

// Global brightness level management
BrightnessLevel global_brightness = DEFAULT_BRIGHTNESS;

// Forward declarations
void configModeCallback(WiFiManager *myWiFiManager);
bool check_button_press();
void smooth_scroll_story();
void select_random_message();
void adjustGlobalBrightness(bool increase);
void showBrightnessAnnouncement();

// Time 
tm timeinfo;
time_t now;
int hour = 0;
int minute = 0;
int second = 0;

// Time, date, and tracking state
int t = 0;
int number = 0;
int animation=0;
String formattedDate;
String dayStamp;
long millis_offset=0;
int last_hour=0;


#define USER_BUTTON 0

// Display configuration - will be managed by DisplayManager
#define NUM_BOARDS 3
#define WIDTH 24
#define HEIGHT 6

#define DRIVER_DEFAULT_BRIGHTNESS 90
#define TEXT_BRIGHT 190        // For time, capitalized words
#define TEXT_DEFAULT_BRIGHTNESS 90  // For normal text
#define TEXT_DIM 30            // For regular lowercase text
#define TEXT_VERY_DIM 12       // For background elements
#define DEMO_MODE_INTERVAL 30000  // 30 seconds between auto mode changes

// IS31FL373x driver - no namespace needed


// ---------------------------------------------------------------------------------------------

// I2C scan moved to DisplayManager::scanI2C()



// I2C functions moved to DisplayManager - using static methods there


// LED drivers will be managed by DisplayManager instance

// LED driver verification moved to DisplayManager::verifyDrivers()


// Legacy LED number calculation - replaced by DisplayManager::mapCoordinateToLED

// Legacy buffer functions - replaced by DisplayManager methods:
// set_led -> DisplayManager::setPixel
// get_led -> DisplayManager::getPixel  
// draw_buffer -> DisplayManager::updateDisplay
// clear_buffer -> DisplayManager::clearBuffer
// dim_buffer -> DisplayManager::dimBuffer


// Character pattern functions moved to DisplayManager::getCharacterPattern

// shift_in_character removed - functionality replaced by SignTextController

// Legacy character drawing functions removed - now handled by SignTextController and DisplayManager

// Check if a word starting at position is all capitals
bool is_word_capitalized(String text, int start_pos) {
  int pos = start_pos;
  bool has_letters = false;
  
  // Check characters until space or end of string
  while (pos < text.length() && text.charAt(pos) != ' ') {
    char c = text.charAt(pos);
    if (c >= 'A' && c <= 'Z') {
      has_letters = true;  // Found at least one capital letter
    } else if (c >= 'a' && c <= 'z') {
      return false;  // Found lowercase letter, word is not all caps
    }
    pos++;
  }
  
  return has_letters;  // Word is capitalized if it has letters and no lowercase
}

// Determine brightness level based on character and context
uint8_t get_character_brightness(char c, String text, int char_pos, bool is_time_display = false) {
  if (is_time_display) {
    // Time display: bright for time (last 8 characters), dim for date
    int time_start_pos = text.length() - 8;  // Time starts at position for "12:43:25"
    if (char_pos >= time_start_pos) {
      return TEXT_BRIGHT;  // Time portion is bright
    }
    return TEXT_DIM;  // Date portion is dim
  }
  
  // Find the start of the current word
  int word_start = char_pos;
  while (word_start > 0 && text.charAt(word_start - 1) != ' ') {
    word_start--;
  }
  
  // Check if the entire word is capitalized
  if (is_word_capitalized(text, word_start)) {
    return TEXT_BRIGHT;  // Entire capitalized word is bright
  }
  
  // Regular character brightness
  if (c >= '0' && c <= '9') return TEXT_DEFAULT_BRIGHTNESS; // Numbers normal
  return TEXT_DIM;                                     // Everything else dim
}

// write_character_at_offset removed - functionality replaced by SignTextController

// Legacy scrolling functions removed - functionality replaced by SignTextController

// Meteor animation moved to MeteorAnimation class



// Message tracking variables
int current_message_index = 0;
String current_message = "";  // Will be initialized in setup

// Module announcement messages
const String MODULE_ANNOUNCEMENTS[] = {
  "Retro Font",       // MODE_RETRO
  "Modern Font",      // MODE_MODERN  
  "Clock Display",    // MODE_CLOCK
  "Meteor Animation"  // MODE_ANIMATION
};


// Message completion tracking for demo mode
bool message_complete = false;

// Global SignTextController instances for demonstration
RetroText::SignTextController* modern_sign = nullptr;
RetroText::SignTextController* retro_sign = nullptr;

// Callback functions for SignTextController integration
void render_character_callback(uint8_t character, int pixel_offset, uint8_t brightness, bool use_alt_font) {
  if (display_manager) {
    // Get character pattern
    uint8_t pattern[6];
    for (int row = 0; row < 6; row++) {
      pattern[row] = display_manager->getCharacterPattern(character, row, use_alt_font);
    }
    // Draw character using DisplayManager
    display_manager->drawCharacter(pattern, pixel_offset, brightness);
  }
}

void clear_display_callback() {
  if (display_manager) {
    display_manager->clearBuffer();
  }
}

void draw_display_callback() {
  if (display_manager) {
    display_manager->updateDisplay();
  }
}

uint8_t brightness_callback(char c, String text, int char_pos, bool is_time_display) {
  return get_character_brightness(c, text, char_pos, is_time_display);
}

// Initialize the SignTextController instances
void init_sign_controllers() {
  if (!display_manager) {
    Serial.println("Error: DisplayManager not initialized");
    return;
  }
  
  // Create modern font controller
  int max_chars = display_manager->getMaxCharacters();
  int char_width = display_manager->getCharacterWidth();
  Serial.printf("Creating modern sign: %d chars, %d pixels per char\n", max_chars, char_width);
  
  modern_sign = new RetroText::SignTextController(max_chars, char_width);
  modern_sign->setFont(RetroText::MODERN_FONT);
  modern_sign->setScrollStyle(RetroText::SMOOTH);  // Use smooth scrolling
  modern_sign->setScrollSpeed(40);  // Use slower speed like retro
  modern_sign->setCharacterSpacing(1);  // Add 1-pixel spacing for better readability
  modern_sign->setBrightness(TEXT_DEFAULT_BRIGHTNESS);
  
  // Use callbacks instead of DisplayManager directly
  modern_sign->setRenderCallback(render_character_callback);
  modern_sign->setClearCallback(clear_display_callback);
  modern_sign->setDrawCallback(draw_display_callback);
  modern_sign->setBrightnessCallback(brightness_callback);
  
  Serial.printf("Modern sign setup complete with callbacks\n");
  
  // Create retro font controller
  Serial.printf("Creating retro sign: %d chars, %d pixels per char\n", max_chars, char_width);
  
  retro_sign = new RetroText::SignTextController(max_chars, char_width);
  retro_sign->setFont(RetroText::ARDUBOY_FONT);
  retro_sign->setScrollStyle(RetroText::CHARACTER);
  retro_sign->setScrollSpeed(130);
  retro_sign->setBrightness(TEXT_DEFAULT_BRIGHTNESS);
  retro_sign->setRenderCallback(render_character_callback);
  retro_sign->setClearCallback(clear_display_callback);
  retro_sign->setDrawCallback(draw_display_callback);
  retro_sign->setBrightnessCallback(brightness_callback);
  
  Serial.println("SignTextController instances initialized");
}

// Helper function to display a static message using SignTextController
void display_static_message(String message, bool use_modern_font = true, int display_time_ms = 2000) {
  RetroText::SignTextController* sign = use_modern_font ? modern_sign : retro_sign;
  
  // Save the original scroll style
  RetroText::ScrollStyle original_style = sign->getScrollStyle();
  
  // Temporarily set to static for announcement
  sign->setScrollStyle(RetroText::STATIC);
  sign->setMessage(message);
  sign->reset();
  sign->update();
  
  if (display_time_ms > 0) {
    delay(display_time_ms);
  }
  
  // Restore the original scroll style
  sign->setScrollStyle(original_style);
}

// Helper function to display a scrolling message using SignTextController
void display_scrolling_message(String message, bool use_modern_font = true, bool smooth_scroll = true) {
  RetroText::SignTextController* sign = use_modern_font ? modern_sign : retro_sign;
  
  sign->setScrollStyle(smooth_scroll ? RetroText::SMOOTH : RetroText::CHARACTER);
  sign->setMessage(message);
  sign->reset();
  
  // Run until complete
  while (!sign->isComplete()) {
    sign->update();
    delay(10);
  }
}



// Function to select a random message
void select_random_message() {
  current_message = getRandomMessage(current_message_index);
  Serial.printf("Selected message %d: %s\n", current_message_index, current_message.substring(0, 50).c_str());
}





// font_test removed - functionality replaced by SignTextController

// Smooth scrolling story text using SignTextController - adapts to current mode
void smooth_scroll_story() {
  // Use the appropriate controller based on current mode
  RetroText::SignTextController* active_sign = nullptr;
  
  if (current_mode == DisplayMode::RETRO) {
    active_sign = retro_sign;
  } else if (current_mode == DisplayMode::MODERN) {
    active_sign = modern_sign;
  }
  
  if (!active_sign) return;
  
  static String last_message = "";
  
  // Update message if it changed
  if (last_message != current_message) {
    Serial.printf("Setting message: '%s...' (length: %d)\n", 
                  current_message.substring(0, 30).c_str(), 
                  current_message.length());
    active_sign->setMessage(current_message);
    active_sign->reset();
    last_message = current_message;
  }
  
  // Call update every loop - let the controller handle its own timing
  active_sign->update();
    
    // Reset when complete for continuous scrolling
  if (active_sign->isComplete()) {
    active_sign->reset();
  }
}

// font_test_2 removed - functionality replaced by SignTextController


// xy_test and bouncy_ball removed - unused legacy functions

// Clock formatting moved to ClockDisplay::formatClockDisplay and ClockDisplay::update

// WiFi AP mode callback - displays configuration message
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFi config mode");
  Serial.println("AP SSID: " + myWiFiManager->getConfigPortalSSID());
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  
  // Display AP configuration message with smooth scrolling
  String ap_message = "config wifi via AP";
  display_scrolling_message(ap_message, true, true); // Use modern font with smooth scrolling
}


// Display font name with button interrupt capability
void display_font_name_interruptible(String font_name, bool use_alt_font) {
  // Use SignTextController for static display
  RetroText::SignTextController* sign = use_alt_font ? modern_sign : retro_sign;
  sign->setScrollStyle(RetroText::STATIC);
  sign->setMessage(font_name);
  sign->reset();
  sign->update();
  
  // Wait up to 1.5 seconds, but check for button press every 50ms
  for (int i = 0; i < 30; i++) {  // 30 * 50ms = 1500ms
    if (check_button_press()) {
      return;  // Exit early if button pressed
    }
    delay(50);
  }
}


// Button press detection using state-based approach (like working example)
bool check_button_press() {
  static unsigned long buttonPressTime = 0;
  static unsigned long last_successful_press = 0;
  const unsigned long min_press_interval = 500;  // Minimum time between presses
  
  // Handle button input (using proven state-based approach)
  if (digitalRead(USER_BUTTON) == LOW) {
    // Button is currently pressed
    if (buttonPressTime == 0) {
      buttonPressTime = millis(); // Mark the time button was first pressed
      Serial.println("*** BUTTON PRESS DETECTED! - Display paused ***");
      
      // Clear display to show immediate response
      if (display_manager) {
        display_manager->clearBuffer();
        display_manager->updateDisplay();
      }
    }
    // Button is being held - we don't do anything until release
  } else {
    // Button is not pressed (HIGH due to pull-up)
    if (buttonPressTime > 0) {
      // Button was just released
      unsigned long press_duration = millis() - buttonPressTime;
      
      // Check if enough time has passed since last successful press
      if (millis() - last_successful_press > min_press_interval) {
        Serial.printf("Button released after %lu ms - switching mode\n", press_duration);
        last_successful_press = millis();
        buttonPressTime = 0;
        return true;
      } else {
        Serial.println("Button press ignored - too soon after last press");
      }
      
      buttonPressTime = 0; // Reset regardless
    }
  }
  
  return false;
}

// Global brightness control functions
void adjustGlobalBrightness(bool increase) {
  // Calculate new brightness level
  BrightnessLevel new_level;

  if (increase) {
    // Increase by one step (5%)
    int current_index = static_cast<int>(global_brightness);
    if (current_index < BRIGHTNESS_STEPS - 1) {
      new_level = static_cast<BrightnessLevel>(current_index + 1);
    } else {
      new_level = BRIGHTNESS_0_PERCENT;  // Wrap around to 0%
    }
  } else {
    // Decrease by one step (5%)
    int current_index = static_cast<int>(global_brightness);
    if (current_index > 0) {
      new_level = static_cast<BrightnessLevel>(current_index - 1);
    } else {
      new_level = static_cast<BrightnessLevel>(BRIGHTNESS_STEPS - 1);  // Wrap around to 100%
    }
  }

  global_brightness = new_level;

  // Update both display and preset LED brightness
  if (display_manager) {
    display_manager->setBrightnessLevel(global_brightness);
  }
  if (radio_hardware && radio_hardware->isInitialized()) {
    radio_hardware->setBrightnessLevel(global_brightness);
  }

  Serial.printf("Global brightness adjusted to: %d%% (%d)\n",
                static_cast<int>(global_brightness) * 5,
                getBrightnessValue(global_brightness));
}

void showBrightnessAnnouncement() {
  if (!announcement_module) return;

  String brightness_text = "Brightness: " + getBrightnessPercentage(global_brightness);
  announcement_module->show(brightness_text, 1500);  // Show for 1.5 seconds
  Serial.printf("Brightness announcement: %s\n", brightness_text.c_str());
}

// Comprehensive I2C scan to detect all connected devices (shared helper)
void performComprehensiveI2CScan() {
  static const I2CKnownDevice known[] = {
    {0x34, "TCA8418 Keypad Controller"},
    {0x50, "IS31FL3737 Display (GND)"},
    {0x55, "IS31FL3737 Preset LEDs (SCL)"},
    {0x5A, "IS31FL3737 Display (VCC)"},
    {0x5F, "IS31FL3737 Display (SDA)"},
  };

  Serial.println("\n==================================================");
  Serial.println("COMPREHENSIVE I2C DEVICE SCAN");
  Serial.println("Scanning I2C bus (SDA=GPIO21, SCL=GPIO22)...\n");
  int count = scanI2CBus(known, sizeof(known)/sizeof(known[0]));
  Serial.printf("\nScan complete: Found %d I2C devices\n", count);
  Serial.println("Expected devices after hardware fix:");
  for (size_t i = 0; i < sizeof(known)/sizeof(known[0]); i++) {
    Serial.printf("  0x%02X - %s\n", known[i].address, known[i].name);
  }
  Serial.printf("  Total expected: %d devices\n", (int)(sizeof(known)/sizeof(known[0])));
  Serial.println("==================================================");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize I2C with default ESP32 pins (matching working RetroText firmware)
  Serial.println("Initializing I2C with default ESP32 pins...");
  Wire.begin(); // SDA=GPIO21, SCL=GPIO22 (ESP32 defaults)
  Wire.setClock(800000); // use 800 kHz I2C
  Serial.println("I2C initialized: SDA=GPIO21, SCL=GPIO22");
  delay(100); // Allow I2C to stabilize

  // Perform comprehensive I2C scan to detect all devices
  performComprehensiveI2CScan();

  Serial.println("Retrotext Starting");
  Serial.println("Press USER_BUTTON to switch modes (MinFont/AltFont/Clock)");
  
  // Initialize display manager first
  Serial.println("Initializing DisplayManager...");
  display_manager = new DisplayManager(NUM_BOARDS, WIDTH, HEIGHT);
  if (!display_manager->initialize()) {
    Serial.println("FATAL: DisplayManager initialization failed!");
    while(1) delay(1000); // Halt
  }
  
  // Print display configuration and connection info
  display_manager->printDisplayConfiguration();
  
  // Set display brightness to default level (50%)
  display_manager->setBrightnessLevel(DEFAULT_BRIGHTNESS);

  // Initialize global brightness to default level
  global_brightness = DEFAULT_BRIGHTNESS;

  // Verify each driver individually with proper error handling
  if (!display_manager->verifyDrivers()) {
    Serial.println("WARNING: Some display drivers failed verification!");
    Serial.println("Check I2C connections and ADDR pin configuration.");
    Serial.println("System will continue but displays may not work properly.");
    delay(3000); // Give user time to read the warning
  }
  
  // Initialize Clock Display
  Serial.println("Initializing ClockDisplay...");
  clock_display = new ClockDisplay(display_manager, &wifiTimeLib);
  clock_display->initialize();
  
  // Initialize Meteor Animation
  Serial.println("Initializing MeteorAnimation...");
  meteor_animation = new MeteorAnimation(display_manager);
  meteor_animation->initialize();
  
  // Initialize Radio Hardware (keypad and preset LEDs)
  Serial.println("Initializing RadioHardware...");
  radio_hardware = new RadioHardware();
  if (!radio_hardware->initialize()) {
    Serial.println("WARNING: RadioHardware initialization had issues - check connections");
  } else {
    Serial.println("RadioHardware initialized successfully");

    // Set preset LED brightness to default level (only if initialization succeeded)
    if (radio_hardware) {
      radio_hardware->setBrightnessLevel(global_brightness);
    }

    // Initialize AnnouncementModule
    Serial.println("Initializing AnnouncementModule...");
    announcement_module = new AnnouncementModule(display_manager);

    // Initialize PresetHandler
    Serial.println("Initializing PresetHandler...");
    preset_handler = new PresetHandler(radio_hardware, announcement_module);
    if (!preset_handler->initialize()) {
      Serial.println("WARNING: PresetHandler initialization failed");
    } else {
      Serial.println("PresetHandler initialized successfully");
    }

    // Connect PresetHandler to RadioHardware
    radio_hardware->setPresetHandler(preset_handler);
  }
  
  // Initialize messages
  initializeMessages();
  current_message = getMessage(0);  // Start with first message
  
  // Integrated initialization with progress bar and display messages
  Serial.println("Starting initialization sequence...");

  // Initialize SignTextController instances after display manager
  init_sign_controllers();

  // Start progress bar at 0%
  if (radio_hardware) {
    radio_hardware->showProgress(0);
  }

  // Step 1: Display test pattern
  Serial.println("Self Test...");
  if (display_manager) {
    display_manager->showTestPattern();
    delay(250);  // Show test pattern for 1 second
  }

  if (radio_hardware) {
    radio_hardware->showProgress(20);  // 20% - Display test
  }

  // Step 2: WiFi setup
  Serial.println("Setting up WiFi...");
  display_static_message("WiFi Setup", true, 1000);
  if (radio_hardware) {
    radio_hardware->showProgress(40);  // 40% - WiFi setup
  }

  // Set up WiFiManager with callback for AP mode
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);

  // Step 3: WiFi connection attempt
  Serial.println("Attempting WiFi connection...");
  display_static_message("WiFi Connecting", true, 1500);
  if (radio_hardware) {
    radio_hardware->showProgress(60);  // 60% - WiFi attempt
  }

  // Try to connect
  if (wm.autoConnect("RetroText")) {
    Serial.println("WiFi connected, syncing time...");
    display_static_message("WiFi Connected", true, 1000);
    if (radio_hardware) {
      radio_hardware->showProgress(80);  // 80% - WiFi connected
    }

    // Step 4: Time sync
    Serial.println("Syncing time...");
    display_static_message("Syncing Time", true, 500);
    if (radio_hardware) {
      radio_hardware->showProgress(90);  // 90% - Time sync attempt
    }

    if (wifiTimeLib.getNTPtime(10, nullptr)) {
      Serial.println("Time synchronized successfully");
      display_static_message("Time Synced", true, 1000);
      if (radio_hardware) {
        radio_hardware->showProgress(100);  // 100% - Complete
        delay(1000);  // Show completion for 1 second
        radio_hardware->clearAllPresetLEDs();  // Clear progress bar
        radio_hardware->updatePresetLEDs();
      }
    } else {
      Serial.println("Warning: Time sync failed, clock mode may show incorrect time");
      display_static_message("Time Sync Failed", true, 2000);
      if (radio_hardware) {
        radio_hardware->showProgress(95);  // 95% - Time sync failed
        delay(1000);
        radio_hardware->clearAllPresetLEDs();
        radio_hardware->updatePresetLEDs();
      }
    }
  } else {
    Serial.println("Warning: WiFi connection failed, clock mode will not work properly");
    display_static_message("WiFi Failed", true, 2000);
    if (radio_hardware) {
      radio_hardware->showProgress(70);  // 70% - WiFi failed
      delay(1000);
      radio_hardware->clearAllPresetLEDs();
      radio_hardware->updatePresetLEDs();
    }
  }
  
  // Final step: System ready
  Serial.println("Initialization complete, starting demo mode...");
  display_static_message("Ready", true, 500);
  if (radio_hardware) {
    radio_hardware->clearAllPresetLEDs();  // Clear progress bar
    radio_hardware->updatePresetLEDs();
  }

  // Setup button for mode switching
  pinMode(USER_BUTTON, INPUT_PULLUP);

  Serial.println("Starting demo mode...");
}

void loop()
{
  static unsigned long last_fps_report = 0;
  static unsigned long frame_count = 0;
  frame_count++;

  // FPS and status report every 5 seconds
  if (millis() - last_fps_report > 5000) {
    float fps = frame_count * 1000.0 / (millis() - last_fps_report);
  Serial.printf("FPS: %.1f | Mode: %s | Announcement: %s\n",
                 fps, mode_names[static_cast<uint8_t>(current_mode)],
                  (announcement_module && announcement_module->isActive()) ? "active" : "idle");
    last_fps_report = millis();
    frame_count = 0;
  }

  // Update hardware - RadioHardware manages all control interactions
  if (radio_hardware) {
    radio_hardware->update();
  }

  // Check for mode changes from initialization or preset buttons
  if (preset_handler && preset_handler->hasModeChanged()) {
    current_mode = preset_handler->getSelectedMode();
    Serial.printf("Mode: %s\n", mode_names[static_cast<uint8_t>(current_mode)]);

    // Select a new random message when mode changes
    select_random_message();

    preset_handler->clearModeChanged();
  }

  // Update all modules
  if (preset_handler) {
    preset_handler->update();
  }

  if (announcement_module) {
    announcement_module->update();
  }

  // Update current display mode (only if announcement is not active)
  if (!announcement_module || !announcement_module->isActive()) {
    switch (current_mode) {
      case DisplayMode::RETRO:
      case DisplayMode::MODERN:
        smooth_scroll_story();
        break;
      case DisplayMode::CLOCK:
        clock_display->update();
        break;
      case DisplayMode::ANIMATION:
        meteor_animation->update();
        break;
    }
  }
  
  delay(10);
}