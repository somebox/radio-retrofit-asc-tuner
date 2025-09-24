#include "PresetHandler.h"
#include "RadioHardware.h"
#include "AnnouncementModule.h"

// Display modes - must match main.cpp
#define MODE_RETRO 0
#define MODE_MODERN 1  
#define MODE_CLOCK 2
#define MODE_ANIMATION 3

// Use centralized brightness levels
#include "BrightnessLevels.h"

// Preset configuration - Based on RetroText PCB Layout (skip CS5)
const PresetConfig PresetHandler::PRESET_CONFIGS[8] = {
  // ID, Name, Mode, Button(row,col), LED(row,col), is_mode_preset
  {0, "Modern", MODE_MODERN, 0, 0, 0, 0, true},     // Button 0 → LED CS1 (0-based)
  {1, "Retro", MODE_RETRO, 0, 1, 0, 1, true},       // Button 1 → LED CS2
  {2, "Clock", MODE_CLOCK, 0, 2, 0, 2, true},        // Button 2 → LED CS3
  {3, "Animation", MODE_ANIMATION, 0, 3, 0, 3, true}, // Button 3 → LED CS4
  {4, "Preset 4", MODE_RETRO, 0, 5, 0, 5, false},    // Button 4 → LED CS6 (skip CS5)
  {5, "Preset 5", MODE_RETRO, 0, 6, 0, 6, false},    // Button 5 → LED CS7
  {6, "Bright+", MODE_RETRO, 0, 7, 0, 7, false},     // Button 6 → LED CS8 (Brightness Up)
  {7, "Bright-", MODE_RETRO, 0, 8, 0, 8, false}      // Button 7 → LED CS9 (Brightness Down)
};

PresetHandler::PresetHandler(RadioHardware* hardware, AnnouncementModule* announcement)
  : radio_hardware_(hardware)
  , announcement_module_(announcement)
  , current_preset_(-1)
  , held_preset_(-1)
  , previous_preset_(-1)
  , press_time_(0)
  , release_time_(0)
  , last_update_(0)
  , show_display_text_(false)
  , mode_changed_(false)
{
  // Initialize all preset states to idle
  for (int i = 0; i < 8; i++) {
    preset_states_[i] = PRESET_IDLE;
    state_change_times_[i] = 0;
  }

  strcpy(display_text_, "");
}

PresetHandler::~PresetHandler() {
  // Nothing to clean up
}

bool PresetHandler::initialize() {
  if (!radio_hardware_) {
    Serial.println("ERROR: PresetHandler - RadioHardware is null");
    return false;
  }
  
  Serial.println("PresetHandler initialized successfully");
  
  // Set default active preset
  setPresetActive(0);  // Default to Modern mode (preset 0)
  
  return true;
}

void PresetHandler::setAnnouncementModule(AnnouncementModule* announcement) {
  announcement_module_ = announcement;
}

void PresetHandler::handleKeypadEvent(int row, int col, bool pressed) {
  // Find the preset configuration for this button position
  const PresetConfig* config = getPresetByPosition(row, col);
  if (!config) {
    Serial.printf("PresetHandler: No preset found for position row=%d, col=%d\n", row, col);
    return;
  }

  int preset_id = config->id;

  if (pressed) {
    Serial.printf("PresetHandler: Preset %d (%s) pressed\n", preset_id, config->name);

    // Update state
    held_preset_ = preset_id;
    press_time_ = millis();
    updatePresetState(preset_id, PRESET_PRESSED);

    // Check if this is a brightness control button (6 or 7)
    if (preset_id == 6 || preset_id == 7) {
      // Handle brightness control - show brightness feedback instead of preset name
      Serial.printf("PresetHandler: Brightness control button %d pressed\n", preset_id);

      // Show current brightness level as announcement
      extern void showBrightnessAnnouncement();
      showBrightnessAnnouncement();

      return;  // Don't show preset name announcement for brightness buttons
    }

    // Show announcement with long duration (will be held while button pressed)
    if (announcement_module_) {
      announcement_module_->show(String(config->name), 30000); // Show for 30 seconds initially
    }

  } else {
    Serial.printf("PresetHandler: Preset %d (%s) released\n", preset_id, config->name);

    if (held_preset_ == preset_id) {
      // This is the preset that was being held
      release_time_ = millis();
      held_preset_ = -1;

      // Check if this is a brightness control button (6 or 7)
      if (preset_id == 6 || preset_id == 7) {
        // Handle brightness control
        Serial.printf("PresetHandler: Brightness control button %d released\n", preset_id);

        // Determine if brightness should increase or decrease
        bool increase = (preset_id == 6);

        // Call the brightness adjustment function (must be implemented in main.cpp)
        extern void adjustGlobalBrightness(bool increase);
        extern void showBrightnessAnnouncement();

        adjustGlobalBrightness(increase);
        showBrightnessAnnouncement();

        // Don't change preset state for brightness buttons
        return;
      }

      // Set as current active preset
      previous_preset_ = current_preset_;
      current_preset_ = preset_id;

      // Start fade immediately
      updatePresetState(preset_id, PRESET_TRANSITIONING);

      // Clear all other presets to ensure only one is active
      for (int i = 0; i < 8; i++) {
        if (i != preset_id) {
          updatePresetState(i, PRESET_IDLE);
        }
      }

      // Check for mode change
      if (config->is_mode_preset) {
        mode_changed_ = true;
        Serial.printf("Mode change requested: %d (%s)\n", preset_id, config->name);
      }

      // Change announcement duration to 1 second after release
      if (announcement_module_) {
        // The announcement is already showing, just change the duration to 1000ms
        announcement_module_->setDuration(1000);
      }
    }
  }

  // Update LEDs immediately
  updateLEDs();
}

void PresetHandler::update() {
  unsigned long current_time = millis();
  last_update_ = current_time;
  
  // Handle display text timeout
  if (show_display_text_ && release_time_ > 0) {
    if (current_time - release_time_ > DISPLAY_TIMEOUT) {
      show_display_text_ = false;
      Serial.println("PresetHandler: Display text timeout");
    }
  }
  
  // Update preset states based on timing
  for (int i = 0; i < 8; i++) {
    if (preset_states_[i] == PRESET_TRANSITIONING) {
      // Transition from RELEASED to ACTIVE after timeout
      if (current_time - release_time_ > DISPLAY_TIMEOUT) {
        if (i == current_preset_) {
          updatePresetState(i, PRESET_ACTIVE);
        } else {
          updatePresetState(i, PRESET_IDLE);
        }
      }
    }
  }
  
  // Update LEDs to handle any animations
  updateLEDs();

  // Keep announcement active while button is held
  if (held_preset_ >= 0 && announcement_module_) {
    announcement_module_->hold();
  }
}

bool PresetHandler::shouldShowDisplayText() const {
  return show_display_text_;
}

int PresetHandler::getSelectedMode() const {
  if (current_preset_ >= 0) {
    const PresetConfig* config = getPresetConfig(current_preset_);
    if (config && config->is_mode_preset) {
      return config->mode;
    }
  }
  return MODE_RETRO; // Default fallback
}

void PresetHandler::updateLEDs() {
  if (!radio_hardware_) return;
  
  // Always clear and render complete state
  radio_hardware_->clearAllPresetLEDs();
  
  for (int i = 0; i < 8; i++) {
    uint8_t brightness = 0;
    calculateLEDBrightness(i, brightness);
    
    const PresetConfig* config = getPresetConfig(i);
    if (config && brightness > 0) {
      radio_hardware_->setLED(config->led_row, config->led_col, brightness);
    }
  }
  
  radio_hardware_->updatePresetLEDs();
}

void PresetHandler::setPresetActive(int preset_id) {
  if (!isValidPreset(preset_id)) return;
  
  // Update states
  previous_preset_ = current_preset_;
  current_preset_ = preset_id;
  
  // Clear any held state
  held_preset_ = -1;
  show_display_text_ = false;
  
  // Update preset states
  for (int i = 0; i < 8; i++) {
    if (i == preset_id) {
      updatePresetState(i, PRESET_ACTIVE);
    } else {
      updatePresetState(i, PRESET_IDLE);
    }
  }
  
  updateLEDs();

  const PresetConfig* config = getPresetConfig(preset_id);
  if (config) {
    // Trigger mode change if this is a mode preset
    if (config->is_mode_preset) {
      mode_changed_ = true;
      Serial.printf("PresetHandler: Set preset %d (%s) as active - mode change triggered\n",
                    preset_id, config->name);
    } else {
      Serial.printf("PresetHandler: Set preset %d (%s) as active\n",
                    preset_id, config->name);
    }
  }
}

const PresetConfig* PresetHandler::getPresetConfig(int preset_id) {
  if (preset_id >= 0 && preset_id < 8) {
    return &PRESET_CONFIGS[preset_id];
  }
  return nullptr;
}

const PresetConfig* PresetHandler::getPresetByPosition(int row, int col) {
  for (int i = 0; i < 8; i++) {
    const PresetConfig* config = &PRESET_CONFIGS[i];
    if (config->button_row == row && config->button_col == col) {
      return config;
    }
  }
  return nullptr;
}

void PresetHandler::updatePresetState(int preset_id, PresetState new_state) {
  if (!isValidPreset(preset_id)) return;
  
  PresetState old_state = preset_states_[preset_id];
  if (old_state != new_state) {
    preset_states_[preset_id] = new_state;
    state_change_times_[preset_id] = millis();
    
    const PresetConfig* config = getPresetConfig(preset_id);
    Serial.printf("Preset %d (%s): %d->%d\n", preset_id, config ? config->name : "?", old_state, new_state);
  }
}

void PresetHandler::updateDisplayText() {
  if (held_preset_ >= 0) {
    // Check if this is a brightness control button (6 or 7)
    if (held_preset_ == 6 || held_preset_ == 7) {
      // For brightness buttons, don't show their names while held
      strcpy(display_text_, "");
    } else {
      const PresetConfig* config = getPresetConfig(held_preset_);
      if (config) {
        strncpy(display_text_, config->name, sizeof(display_text_) - 1);
        display_text_[sizeof(display_text_) - 1] = '\0';
      }
    }
  } else if (current_preset_ >= 0) {
    const PresetConfig* config = getPresetConfig(current_preset_);
    if (config) {
      strncpy(display_text_, config->name, sizeof(display_text_) - 1);
      display_text_[sizeof(display_text_) - 1] = '\0';
    }
  } else {
    strcpy(display_text_, "");
  }
}

void PresetHandler::calculateLEDBrightness(int preset_id, uint8_t& brightness) {
  if (!isValidPreset(preset_id)) {
    brightness = 0;
    return;
  }
  
  PresetState state = preset_states_[preset_id];
  unsigned long current_time = millis();
  
  switch (state) {
    case PRESET_IDLE:
      brightness = 0;
      break;
      
    case PRESET_PRESSED:
      brightness = getBrightnessValue(BRIGHTNESS_100_PERCENT);  // Full brightness when pressed
      break;

    case PRESET_TRANSITIONING:
      // Fade from bright to dim over time
      if (release_time_ > 0) {
        unsigned long elapsed = current_time - release_time_;
        if (elapsed < FADE_DURATION) {
          // Fade from high to low brightness
          float progress = (float)elapsed / FADE_DURATION;
          uint8_t high_bright = getBrightnessValue(BRIGHTNESS_100_PERCENT);
          uint8_t low_bright = getBrightnessValue(BRIGHTNESS_50_PERCENT);
          int fade_range = high_bright - low_bright;
          brightness = high_bright - (fade_range * progress);
        } else {
          brightness = getBrightnessValue(BRIGHTNESS_50_PERCENT);  // Dimmed brightness
        }
      } else {
        brightness = getBrightnessValue(BRIGHTNESS_50_PERCENT);
      }
      break;

    case PRESET_ACTIVE:
      brightness = getBrightnessValue(BRIGHTNESS_50_PERCENT);  // Dimmed brightness for active preset
      break;
      
    default:
      brightness = 0;
      break;
  }
}

bool PresetHandler::isValidPreset(int preset_id) const {
  return preset_id >= 0 && preset_id < 8;
}
