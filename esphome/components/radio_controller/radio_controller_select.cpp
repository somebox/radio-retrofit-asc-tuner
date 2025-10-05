#include "radio_controller_select.h"
#include "radio_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace radio_controller {

static const char *const TAG = "radio_controller.select";

void RadioControllerSelect::setup() {
  // Get preset names from parent and set as options
  if (this->parent_ != nullptr) {
    auto names = this->parent_->get_preset_names();
    this->traits.set_options(names);
    
    ESP_LOGCONFIG(TAG, "Radio Controller Select initialized with %d presets", names.size());
  }
}

void RadioControllerSelect::control(const std::string &value) {
  // User selected a preset from Home Assistant
  ESP_LOGD(TAG, "Select control: '%s'", value.c_str());
  
  if (this->parent_ != nullptr) {
    this->parent_->select_preset_by_name(value);
    this->publish_state(value);
  }
}

}  // namespace radio_controller
}  // namespace esphome
