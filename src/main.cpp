#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>

#include "WifiTimeLib.h"
#include "DisplayManager.h"
#include "ClockDisplay.h"
#include "MeteorAnimation.h"
#include "RadioHardware.h"
#include "PresetManager.h"
#include "AnnouncementModule.h"
#include "Events.h"
#include "HomeAssistantBridge.h"
#include "I2CScan.h"
#include "messages.h"
#include "SignTextController.h"
#include "DisplayMode.h"

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

// Mode system
static const char* mode_names[] = {"Retro", "Modern", "Clock", "Animation"};
DisplayMode current_mode = DisplayMode::MODERN;

// Global brightness level management
uint8_t global_brightness = 128;

// Forward declarations
void configModeCallback(WiFiManager *myWiFiManager);
void adjustGlobalBrightness(bool increase);
void showBrightnessAnnouncement();
void select_random_message();

// SignTextController instances for demonstration
std::unique_ptr<RetroText::SignTextController> modern_sign;
std::unique_ptr<RetroText::SignTextController> retro_sign;

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

// Text formatting helper
bool is_word_capitalized(String text, int start_pos) {
  int pos = start_pos;
  bool has_letters = false;
  
  while (pos < text.length() && text.charAt(pos) != ' ') {
    char c = text.charAt(pos);
    if (c >= 'A' && c <= 'Z') {
      has_letters = true;
    } else if (c >= 'a' && c <= 'z') {
      return false;
    }
    pos++;
  }
  
  return has_letters;
}

// Determine brightness level based on character and context
uint8_t get_character_brightness(char c, String text, int char_pos, bool is_time_display = false) {
  const uint8_t TEXT_BRIGHT = 190;
  const uint8_t TEXT_DEFAULT_BRIGHTNESS = 90;
  const uint8_t TEXT_DIM = 30;
  
  if (is_time_display) {
    int time_start_pos = text.length() - 8;  // Time starts at position for "12:43:25"
    return (char_pos >= time_start_pos) ? TEXT_BRIGHT : TEXT_DIM;
  }
  
  // Find the start of the current word
  int word_start = char_pos;
  while (word_start > 0 && text.charAt(word_start - 1) != ' ') {
    word_start--;
  }
  
  // Check if the current word is capitalized
  if (is_word_capitalized(text, word_start)) {
    return TEXT_BRIGHT;  // Capitalized words are bright
  }
  
  return (c >= 'a' && c <= 'z') ? TEXT_DIM : TEXT_DEFAULT_BRIGHTNESS;
}

// Callback functions for SignTextController integration
void render_character_callback(uint8_t character, int pixel_offset, uint8_t brightness, bool use_alt_font) {
  if (display_manager) {
    uint8_t pattern[6];
    for (int row = 0; row < 6; row++) {
      pattern[row] = display_manager->getCharacterPattern(character, row, use_alt_font);
    }
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
  
  int max_chars = display_manager->getMaxCharacters();
  int char_width = display_manager->getCharacterWidth();
  
  // Create modern font controller
  modern_sign = std::make_unique<RetroText::SignTextController>(max_chars, char_width);
  modern_sign->setFont(RetroText::MODERN_FONT);
  modern_sign->setScrollStyle(RetroText::SMOOTH);
  modern_sign->setScrollSpeed(40);
  modern_sign->setCharacterSpacing(1);
  modern_sign->setBrightness(90);
  modern_sign->setRenderCallback(render_character_callback);
  modern_sign->setClearCallback(clear_display_callback);
  modern_sign->setDrawCallback(draw_display_callback);
  modern_sign->setBrightnessCallback(brightness_callback);
  
  // Create retro font controller
  retro_sign = std::make_unique<RetroText::SignTextController>(max_chars, char_width);
  retro_sign->setFont(RetroText::ARDUBOY_FONT);
  retro_sign->setScrollStyle(RetroText::CHARACTER);
  retro_sign->setScrollSpeed(130);
  retro_sign->setBrightness(90);
  retro_sign->setRenderCallback(render_character_callback);
  retro_sign->setClearCallback(clear_display_callback);
  retro_sign->setDrawCallback(draw_display_callback);
  retro_sign->setBrightnessCallback(brightness_callback);
  
  Serial.println("SignTextController instances initialized");
}

// Helper function to display a static message
void display_static_message(String message, bool use_modern_font, int duration_ms) {
  RetroText::SignTextController* sign = use_modern_font ? modern_sign.get() : retro_sign.get();
  if (sign) {
    sign->setScrollStyle(RetroText::STATIC);
    sign->setMessage(message);
    sign->reset();
    sign->update();
  }
  delay(duration_ms);
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize I2C
  Serial.println("Initializing I2C with default ESP32 pins...");
  Wire.begin(); // SDA=GPIO21, SCL=GPIO22 (ESP32 defaults)
  Wire.setClock(800000); // use 800 kHz I2C
  Serial.println("I2C initialized: SDA=GPIO21, SCL=GPIO22");
  delay(100);

  Serial.println("Retrotext Starting");
  
  // Initialize display manager first
  Serial.println("Initializing DisplayManager...");
  display_manager = new DisplayManager(NUM_BOARDS, WIDTH, HEIGHT);
  if (!display_manager->initialize()) {
    Serial.println("FATAL: DisplayManager initialization failed!");
    while(1) delay(1000);
  }
  
  display_manager->printDisplayConfiguration();
  display_manager->scanI2C();
  
  global_brightness = 128;
  display_manager->setBrightnessLevel(global_brightness);

  if (!display_manager->verifyDrivers()) {
    Serial.println("WARNING: Some display drivers failed verification!");
    delay(3000);
  }
  
  // Initialize modules
  Serial.println("Initializing ClockDisplay...");
  clock_display = new ClockDisplay(display_manager, &wifiTimeLib);
  clock_display->initialize();
  
  Serial.println("Initializing MeteorAnimation...");
  meteor_animation = new MeteorAnimation(display_manager);
  meteor_animation->initialize();
  
  // Initialize Home Assistant Bridge
  Serial.println("Initializing HomeAssistant Bridge...");
  home_assistant_bridge = new HomeAssistantBridge();
  home_assistant_bridge->begin();

  // Initialize Radio Hardware
  Serial.println("Initializing RadioHardware...");
  radio_hardware = new RadioHardware();
  radio_hardware->setEventBus(&eventBus());
  radio_hardware->setBridge(home_assistant_bridge);
  if (!radio_hardware->initialize()) {
    Serial.println("WARNING: RadioHardware initialization had issues");
  } else {
    Serial.println("RadioHardware initialized successfully");
    radio_hardware->setGlobalBrightness(global_brightness);

    // Initialize AnnouncementModule and PresetManager
    Serial.println("Initializing AnnouncementModule...");
    announcement_module = new AnnouncementModule(display_manager);

    Serial.println("Initializing PresetManager...");
    preset_manager = new PresetManager(radio_hardware, announcement_module);
    if (!preset_manager->initialize()) {
      Serial.println("WARNING: PresetManager initialization failed");
    } else {
      Serial.println("PresetManager initialized successfully");
    }
  }
  
  // Initialize messages and sign controllers
  initializeMessages();
  current_message = getMessage(0);
  init_sign_controllers();

  // Show initialization progress
  if (radio_hardware) {
    radio_hardware->showProgress(0);
  }

  // Display test pattern
  Serial.println("Self Test...");
  if (display_manager) {
    display_manager->showTestPattern();
    delay(250);
  }
  if (radio_hardware) {
    radio_hardware->showProgress(20);
  }

  // WiFi setup
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
  
  Serial.println("Initialization complete, starting demo mode...");
  display_static_message("Ready", true, 500);
  if (radio_hardware) {
    radio_hardware->clearAllPresetLEDs();
    radio_hardware->updatePresetLEDs();
  }

  pinMode(USER_BUTTON, INPUT_PULLUP);
  Serial.println("Starting demo mode...");
}

void loop() {
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

  // Update hardware and bridge
  if (home_assistant_bridge) {
    home_assistant_bridge->update();
  }
  if (radio_hardware) {
    radio_hardware->update();
  }

  // Check for mode changes
  if (preset_manager && preset_manager->hasModeChanged()) {
    current_mode = preset_manager->getSelectedMode();
    Serial.printf("Mode: %s\n", mode_names[static_cast<uint8_t>(current_mode)]);
    select_random_message();
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
          retro_sign->setMessage(current_message);
          retro_sign->update();
        }
        break;
        
      case DisplayMode::MODERN:
        if (modern_sign) {
          modern_sign->setMessage(current_message);
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
