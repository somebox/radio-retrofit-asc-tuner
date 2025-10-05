#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace radio_controller {

// Forward declaration
class RadioController;

class RadioControllerSelect : public select::Select, public Component {
 public:
  void setup() override;
  void set_parent(RadioController *parent) { this->parent_ = parent; }
  
 protected:
  void control(const std::string &value) override;
  
  RadioController *parent_{nullptr};
};

}  // namespace radio_controller
}  // namespace esphome
