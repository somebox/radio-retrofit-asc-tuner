#include <stdint.h>
#include <Arduino.h>
#pragma once

#include "display/DisplayMode.h"
#include "platform/events/Events.h"

// Forward declarations
class RadioHardware;
class AnnouncementModule;

enum class PresetContext : uint8_t {
  Default = 0,
  Menu = 1,
};

enum class PresetAction : uint8_t {
  None = 0,
  SelectMode,
  BrightnessDelta,
  EnterMenu,
  ExitMenuSave,
};

struct PresetButtonBinding {
  PresetAction action{PresetAction::None};
  int value{0};
  const char* label{nullptr};
  bool is_mode_preset{false};
};

enum PresetState {
  PRESET_IDLE = 0,
  PRESET_PRESSED = 1,
  PRESET_TRANSITIONING = 2,
  PRESET_ACTIVE = 3,
  PRESET_DISABLED = 4
};

class PresetManager {
public:
  static constexpr std::size_t kButtonCount = 9;  // 0-8 (memory button)

  PresetManager(RadioHardware* hardware, AnnouncementModule* announcement = nullptr);
  ~PresetManager();

  bool initialize();
  void setAnnouncementModule(AnnouncementModule* announcement);
  void update();

  DisplayMode getSelectedMode() const;
  bool hasModeChanged() const { return mode_changed_; }
  void clearModeChanged() { mode_changed_ = false; }
  PresetContext context() const { return context_; }

private:
  struct PresetConfig {
    int id;
    int button_row;
    int button_col;
    int led_row;
    int led_col;
  };

  RadioHardware* radio_hardware_;
  AnnouncementModule* announcement_module_;

  PresetContext context_;
  DisplayMode current_mode_;
  int active_button_;
  int held_button_;
  bool mode_changed_;

  PresetState button_states_[kButtonCount];
  unsigned long state_change_times_[kButtonCount];
  unsigned long press_times_[kButtonCount];

  unsigned long release_time_;
  unsigned long last_update_;

  bool handlers_registered_;

  const PresetButtonBinding* bindings_;  // current context bindings

  bool registerEventHandlers();
  void unregisterEventHandlers();

  void handlePresetEvent(EventType type, int row, int col);
  void handleButtonPressed(int button_index);
  void handleButtonReleased(int button_index);
  void enterMenu();
  void exitMenu(bool save);

  void updateLEDs();
  void updateButtonState(int button_index, PresetState new_state);
  void applyAction(int button_index, const PresetButtonBinding& binding, bool long_press);
  void announce(const char* text, unsigned long duration_ms = 1000);

  static const PresetConfig BUTTON_CONFIG[kButtonCount];
  static const PresetButtonBinding DEFAULT_BINDINGS[kButtonCount];
  static const PresetButtonBinding MENU_BINDINGS[kButtonCount];

  static constexpr unsigned long DISPLAY_TIMEOUT = 1000;
  static constexpr unsigned long FADE_DURATION = 300;
  static constexpr unsigned long LONG_PRESS_THRESHOLD = 600;  // ms

  static void handlePresetPressedEvent(const events::Event& event, void* context);
  static void handlePresetReleasedEvent(const events::Event& event, void* context);
};


