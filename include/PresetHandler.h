#pragma once

#include <Arduino.h>
#include "DisplayMode.h"

// Forward declarations
class RadioHardware;
class AnnouncementModule;

// Preset configuration structure
struct PresetConfig {
  int id;                    // Preset ID (0-7)
  const char* name;          // Display name
  DisplayMode mode;          // Associated display mode
  int button_row;            // Keypad row
  int button_col;            // Keypad column  
  int led_row;               // LED matrix row (SW pin)
  int led_col;               // LED matrix column (CS pin)
  bool is_mode_preset;       // True if this preset switches modes
};

// Preset button states
enum PresetState {
  PRESET_IDLE = 0,           // Not pressed
  PRESET_PRESSED = 1,        // Currently pressed (LED bright)
  PRESET_TRANSITIONING = 2,  // Transitioning after release
  PRESET_ACTIVE = 3,         // Active preset (LED dimmed)
  PRESET_DISABLED = 4        // Temporarily disabled
};

class PresetHandler {
public:
  PresetHandler(RadioHardware* hardware, AnnouncementModule* announcement = nullptr);
  ~PresetHandler();
  
  // Initialization
  bool initialize();
  void setAnnouncementModule(AnnouncementModule* announcement);
  
  // Event handling
  void handleKeypadEvent(int row, int col, bool pressed);
  
  // State management
  void update();  // Call this regularly to handle animations and timeouts

  // Getters
  int getCurrentPreset() const { return current_preset_; }
  int getHeldPreset() const { return held_preset_; }
  const char* getDisplayText() const { return display_text_; }
  bool shouldShowDisplayText() const;
  DisplayMode getSelectedMode() const;  // Returns mode associated with current preset
  bool hasModeChanged() const { return mode_changed_; }
  void clearModeChanged() { mode_changed_ = false; }
  
  // LED control
  void updateLEDs();
  void setPresetActive(int preset_id);
  
  // Configuration
  static const PresetConfig* getPresetConfig(int preset_id);
  static const PresetConfig* getPresetByPosition(int row, int col);
  
private:
  RadioHardware* radio_hardware_;
  AnnouncementModule* announcement_module_;
  
  // State tracking
  int current_preset_;           // Currently active preset (-1 if none)
  int held_preset_;              // Currently held preset (-1 if none)
  int previous_preset_;          // Previously active preset
  PresetState preset_states_[8]; // State for each preset
  unsigned long state_change_times_[8]; // When each preset changed state
  
  // Timing
  unsigned long press_time_;     // When current preset was pressed
  unsigned long release_time_;   // When current preset was released
  unsigned long last_update_;    // Last update time for animations
  
  // Display
  char display_text_[32];        // Current display text
  bool show_display_text_;       // Whether to show display text

  // Mode management
  bool mode_changed_;            // Flag when mode changes
  
  // Animation parameters
  static const unsigned long DISPLAY_TIMEOUT = 1000;  // 1 second
  static const unsigned long FADE_DURATION = 300;     // 300ms fade
  
  // Internal methods
  void updatePresetState(int preset_id, PresetState new_state);
  void updateDisplayText();
  void calculateLEDBrightness(int preset_id, uint8_t& brightness);
  bool isValidPreset(int preset_id) const;
  
  // Preset configuration - defined in cpp file
  static const PresetConfig PRESET_CONFIGS[8];
};
