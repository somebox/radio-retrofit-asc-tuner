#include "hardware/PresetManager.h"
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

const PresetManager::PresetConfig PresetManager::BUTTON_CONFIG[kButtonCount] = {
  {0, 0, 0, 0, 0},
  {1, 0, 1, 0, 1},
  {2, 0, 2, 0, 2},
  {3, 0, 3, 0, 3},
  {4, 0, 4, 0, 4},
  {5, 0, 5, 0, 5},
  {6, 0, 6, 0, 6},
  {7, 0, 7, 0, 7},
  {8, 0, 8, 0, 8},
};

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
  , handlers_registered_(false)
  , bindings_(DEFAULT_BINDINGS)
{
  for (std::size_t i = 0; i < kButtonCount; ++i) {
    button_states_[i] = PRESET_IDLE;
    state_change_times_[i] = 0;
    press_times_[i] = 0;
  }
}

PresetManager::~PresetManager() {
  unregisterEventHandlers();
}

bool PresetManager::initialize() {
  if (!radio_hardware_) {
    Serial.println("PresetManager: RadioHardware is null");
    return false;
  }

  updateButtonState(active_button_, PRESET_ACTIVE);
  updateLEDs();
 
  if (!registerEventHandlers()) {
    Serial.println("PresetManager: Event handler registration failed");
    return false;
  }

  Serial.println("PresetManager initialized");
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

bool PresetManager::registerEventHandlers() {
  if (handlers_registered_) {
    return true;
  }

  EventBus& bus = eventBus();
  bool ok = true;
  ok &= bus.subscribe(EventType::PresetPressed, &PresetManager::handlePresetPressedEvent, this);
  ok &= bus.subscribe(EventType::PresetReleased, &PresetManager::handlePresetReleasedEvent, this);

  handlers_registered_ = ok;
  return ok;
}

void PresetManager::unregisterEventHandlers() {
  if (!handlers_registered_) {
    return;
  }

  EventBus& bus = eventBus();
  bus.unsubscribe(EventType::PresetPressed, &PresetManager::handlePresetPressedEvent, this);
  bus.unsubscribe(EventType::PresetReleased, &PresetManager::handlePresetReleasedEvent, this);
  handlers_registered_ = false;
}

void PresetManager::handlePresetEvent(EventType type, int row, int col) {
  if (row != 0) {
    return;
  }

  if (col < 0 || col >= static_cast<int>(kButtonCount)) {
    return;
  }

  const int button_index = col;

  if (type == EventType::PresetPressed) {
    handleButtonPressed(button_index);
  } else if (type == EventType::PresetReleased) {
    handleButtonReleased(button_index);
  }
}

void PresetManager::handleButtonPressed(int button_index) {
  held_button_ = button_index;
  press_times_[button_index] = millis();
  updateButtonState(button_index, PRESET_PRESSED);

  const PresetButtonBinding& binding = bindings_[button_index];
  if (binding.label) {
    announce(binding.label, 500);  // quick feedback on press
  }

  if (announcement_module_) {
    announcement_module_->hold();
  }
}

void PresetManager::handleButtonReleased(int button_index) {
  bool was_held = (held_button_ == button_index);
  held_button_ = -1;
  release_time_ = millis();

  bool long_press = (release_time_ - press_times_[button_index]) >= LONG_PRESS_THRESHOLD;
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
        mode_changed_ = true;
        current_mode_ = static_cast<DisplayMode>(binding.value);
        if (previous >= 0 && previous < static_cast<int>(kButtonCount)) {
          updateButtonState(previous, PRESET_IDLE);
        }
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

  for (std::size_t i = 0; i < kButtonCount; ++i) {
    uint8_t brightness = 0;
    switch (button_states_[i]) {
      case PRESET_IDLE:
        brightness = 0;
        break;
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

    if (brightness > 0) {
      const auto& cfg = BUTTON_CONFIG[i];
      radio_hardware_->setLED(cfg.led_row, cfg.led_col, brightness);
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

void PresetManager::handlePresetPressedEvent(const events::Event& event, void* context) {
  if (!context) {
    return;
  }

  auto* self = static_cast<PresetManager*>(context);
  
  // Parse JSON payload: {"value":col,"aux":row}
  String json_str = String(event.value.c_str());
  int col = parseIntField(json_str, "value", -1);
  int row = parseIntField(json_str, "aux", 0);
  
  self->handlePresetEvent(EventType::PresetPressed, row, col);
}

void PresetManager::handlePresetReleasedEvent(const events::Event& event, void* context) {
  if (!context) {
    return;
  }

  auto* self = static_cast<PresetManager*>(context);
  
  // Parse JSON payload: {"value":col,"aux":row}
  String json_str = String(event.value.c_str());
  int col = parseIntField(json_str, "value", -1);
  int row = parseIntField(json_str, "aux", 0);
  
  self->handlePresetEvent(EventType::PresetReleased, row, col);
}


