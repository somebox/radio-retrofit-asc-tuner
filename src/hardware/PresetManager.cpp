#include "hardware/PresetManager.h"
#include "hardware/HardwareConfig.h"
#include "features/AnnouncementModule.h"
#include "hardware/RadioHardware.h"
#include "platform/JsonHelpers.h"

namespace {

// Simple JSON field parsing (shared with HomeAssistantBridge)
int parseIntField(const String& line, const char* key, int fallback) {
  String pattern = String("\"") + key + "\":";
  int idx = line.indexOf(pattern);
  if (idx == -1) return fallback;
  int start = idx + pattern.length();
  int end = line.indexOf(',', start);
  if (end == -1) end = line.indexOf('}', start);
  if (end == -1) return fallback;
  return line.substring(start, end).toInt();
}

constexpr uint8_t buttonToIndex(int row, int col) {
  return static_cast<uint8_t>(row == 0 ? col : 0xFF);
}

const char* modeName(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::RETRO: return "Retro";
    case DisplayMode::MODERN: return "Modern";
    case DisplayMode::CLOCK: return "Clock";
    case DisplayMode::ANIMATION: return "Animation";
  }
  return "Unknown";
}

}  // namespace

const PresetButtonBinding PresetManager::DEFAULT_BINDINGS[kButtonCount] = {
  {PresetAction::SelectMode, static_cast<int>(DisplayMode::MODERN), "Modern", true},
  {PresetAction::SelectMode, static_cast<int>(DisplayMode::RETRO), "Retro", true},
  {PresetAction::SelectMode, static_cast<int>(DisplayMode::CLOCK), "Clock", true},
  {PresetAction::SelectMode, static_cast<int>(DisplayMode::ANIMATION), "Animation", true},
  {PresetAction::None, 0, "Preset 4", false},
  {PresetAction::None, 0, "Preset 5", false},
  {PresetAction::BrightnessDelta, +1, "Bright +", false},
  {PresetAction::BrightnessDelta, -1, "Bright -", false},
  {PresetAction::EnterMenu, 0, "Menu", false},
};

const PresetButtonBinding PresetManager::MENU_BINDINGS[kButtonCount] = {
  {PresetAction::None, 0, "Unused", false},
  {PresetAction::None, 0, "Unused", false},
  {PresetAction::None, 0, "Unused", false},
  {PresetAction::None, 0, "Unused", false},
  {PresetAction::None, 0, "Preset 4", false},
  {PresetAction::None, 0, "Preset 5", false},
  {PresetAction::BrightnessDelta, +1, "Bright +", false},
  {PresetAction::BrightnessDelta, -1, "Bright -", false},
  {PresetAction::ExitMenuSave, 0, "Save", false},
};

PresetManager::PresetManager(RadioHardware* hardware, AnnouncementModule* announcement)
  : radio_hardware_(hardware)
  , announcement_module_(announcement)
  , context_(PresetContext::Default)
  , current_mode_(DisplayMode::MODERN)
  , active_button_(0)
  , held_button_(-1)
  , mode_changed_(false)
  , release_time_(0)
  , last_update_(0)
  , bindings_(DEFAULT_BINDINGS)
{
  for (std::size_t i = 0; i < kButtonCount; ++i) {
    button_states_[i] = PRESET_IDLE;
    state_change_times_[i] = 0;
  }
}

PresetManager::~PresetManager() {
  // No cleanup needed
}

bool PresetManager::initialize() {
  if (!radio_hardware_) {
    Serial.println("PresetManager: RadioHardware is null");
    return false;
  }

  updateButtonState(active_button_, PRESET_ACTIVE);
  updateLEDs();

  Serial.println("PresetManager initialized (using InputManager)");
  return true;
}

void PresetManager::setAnnouncementModule(AnnouncementModule* announcement) {
  announcement_module_ = announcement;
}

DisplayMode PresetManager::getSelectedMode() const {
  return current_mode_;
}

void PresetManager::update() {
  last_update_ = millis();

  // Check all buttons via InputManager
  checkButtons();

  // Handle fade transitions
  for (std::size_t i = 0; i < kButtonCount; ++i) {
    if (button_states_[i] == PRESET_TRANSITIONING) {
      if (millis() - release_time_ > FADE_DURATION) {
        updateButtonState(i, (static_cast<int>(i) == active_button_) ? PRESET_ACTIVE : PRESET_IDLE);
      }
    }
  }

  updateLEDs();

  if (held_button_ >= 0 && announcement_module_) {
    announcement_module_->hold();
  }
}

void PresetManager::checkButtons() {
  if (!radio_hardware_) {
    return;
  }

  auto& input = radio_hardware_->inputManager();

  for (int i = 0; i < static_cast<int>(kButtonCount); ++i) {
    if (!input.hasButton(i)) {
      continue;
    }

    const auto& btn = input.button(i);

    // Handle press
    if (btn.wasJustPressed()) {
      handleButtonPressed(i);
    }

    // Handle release
    if (btn.wasJustReleased()) {
      bool long_press = btn.wasLongPress(LONG_PRESS_THRESHOLD);
      handleButtonReleased(i, long_press);
    }
  }
}

void PresetManager::handleButtonPressed(int button_index) {
  held_button_ = button_index;
  updateButtonState(button_index, PRESET_PRESSED);

  // Log preset button press using the name from HardwareConfig
  using namespace HardwareConfig;
  const auto* preset_btn = getPresetButton(button_index);
  const auto* preset_led = getPresetLED(button_index);
  if (preset_btn && preset_led) {
    Serial.printf("[PresetManager] Button index %d: %s (LED: SW%d CS%d)\n", 
                  button_index, preset_btn->name, preset_led->sw_pin, preset_led->cs_pin);
  } else {
    Serial.printf("[PresetManager] Button %d pressed (config error!)\n", button_index);
  }

  const PresetButtonBinding& binding = bindings_[button_index];
  if (binding.label) {
    announce(binding.label, 500);  // quick feedback on press
  }

  if (announcement_module_) {
    announcement_module_->hold();
  }
}

void PresetManager::handleButtonReleased(int button_index, bool long_press) {
  bool was_held = (held_button_ == button_index);
  held_button_ = -1;
  release_time_ = millis();

  // Log preset button release using the name from HardwareConfig
  using namespace HardwareConfig;
  const auto* preset_btn = getPresetButton(button_index);
  if (preset_btn) {
    Serial.printf("%s released (%s)\n", preset_btn->name, long_press ? "long" : "short");
  } else {
    Serial.printf("Button %d released (%s)\n", button_index, long_press ? "long" : "short");
  }

  updateButtonState(button_index, PRESET_TRANSITIONING);

  const PresetButtonBinding& binding = bindings_[button_index];
  applyAction(button_index, binding, long_press);

  if (announcement_module_) {
    if (was_held) {
      announcement_module_->setDuration(200);
      announcement_module_->hold();
    }
    announce(binding.label ? binding.label : modeName(current_mode_), was_held ? 200 : 800);
  }
}

void PresetManager::applyAction(int button_index, const PresetButtonBinding& binding, bool long_press) {
  switch (binding.action) {
    case PresetAction::SelectMode: {
      int previous = active_button_;
      active_button_ = button_index;
      if (previous != active_button_) {
        Serial.printf("Mode change: button %d -> button %d (%s)\n", 
                      previous, active_button_, modeName(static_cast<DisplayMode>(binding.value)));
        mode_changed_ = true;
        current_mode_ = static_cast<DisplayMode>(binding.value);
        if (previous >= 0 && previous < static_cast<int>(kButtonCount)) {
          updateButtonState(previous, PRESET_IDLE);
          Serial.printf("  Previous button %d set to IDLE\n", previous);
        }
        
        // Publish preset selection to ESPHome for automations
        events::Event evt(EventType::ModeChanged);
        evt.timestamp = millis();
        evt.value = events::json::object({
            events::json::number_field("value", static_cast<int>(current_mode_)),
            events::json::string_field("name", modeName(current_mode_)),
            events::json::number_field("preset", button_index),
        });
        eventBus().publish(evt);
      }
      break;
    }
    case PresetAction::BrightnessDelta: {
      extern void adjustGlobalBrightness(bool increase);
      extern void showBrightnessAnnouncement();
      adjustGlobalBrightness(binding.value > 0);
      showBrightnessAnnouncement();
      events::Event evt(EventType::BrightnessChanged);
      evt.timestamp = millis();
      evt.value = events::json::object({
          events::json::number_field("value", binding.value),
      });
      eventBus().publish(evt);
      break;
    }
    case PresetAction::EnterMenu: {
      if (long_press) {
        enterMenu();
        events::Event evt(EventType::ModeChanged);
        evt.timestamp = millis();
        evt.value = events::json::object({
            events::json::string_field("value", std::to_string(static_cast<int>(context_))),
        });
        eventBus().publish(evt);
      }
      break;
    }
    case PresetAction::ExitMenuSave: {
      exitMenu(true);
      events::Event evt(EventType::ModeChanged);
      evt.timestamp = millis();
      evt.value = events::json::object({
          events::json::string_field("value", std::to_string(static_cast<int>(context_))),
      });
      eventBus().publish(evt);
      break;
    }
    case PresetAction::None:
    default:
      break;
  }

  updateLEDs();
}

void PresetManager::enterMenu() {
  context_ = PresetContext::Menu;
  bindings_ = MENU_BINDINGS;
  announce("Menu", 1000);
}

void PresetManager::exitMenu(bool save) {
  context_ = PresetContext::Default;
  bindings_ = DEFAULT_BINDINGS;
  announce(save ? "Saved" : "Cancel", 1000);
  updateButtonState(active_button_, PRESET_ACTIVE);
}

void PresetManager::updateLEDs() {
  if (!radio_hardware_) {
    return;
  }

  radio_hardware_->clearAllPresetLEDs();

  // Only light the active preset LED, all others should be off
  // This ensures clean state - only one LED lit at a time for mode presets
  for (std::size_t i = 0; i < kButtonCount && i < HardwareConfig::NUM_PRESETS; ++i) {
    uint8_t brightness = 0;
    
    // Only the active button should be lit
    // PRESSED state during button press, TRANSITIONING during fade, ACTIVE when settled
    if (static_cast<int>(i) == active_button_) {
      switch (button_states_[i]) {
        case PRESET_PRESSED:
          brightness = 255;
          break;
        case PRESET_TRANSITIONING:
          brightness = 128;
          break;
        case PRESET_ACTIVE:
          brightness = 128;
          break;
        default:
          brightness = 0;
          break;
      }
    }
    // All non-active buttons should be off (forced to 0)

    if (brightness > 0) {
      // Get LED position from HardwareConfig
      const auto* led = HardwareConfig::getPresetLED(i);
      if (led) {
        radio_hardware_->setLED(led->sw_pin, led->cs_pin, brightness);
      }
    }
  }

  radio_hardware_->updatePresetLEDs();
}

void PresetManager::updateButtonState(int button_index, PresetState new_state) {
  if (button_index < 0 || button_index >= static_cast<int>(kButtonCount)) {
    return;
  }

  if (button_states_[button_index] != new_state) {
    button_states_[button_index] = new_state;
    state_change_times_[button_index] = millis();
  }
}

void PresetManager::announce(const char* text, unsigned long duration_ms) {
  if (!announcement_module_ || !text) {
    return;
  }

  announcement_module_->show(String(text), duration_ms);
}

// Event handlers removed - PresetManager now queries InputManager directly


