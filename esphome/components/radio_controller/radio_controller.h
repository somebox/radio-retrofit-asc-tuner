#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace radio_controller {

class RadioController : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_mode(uint8_t mode);
  void set_volume(uint8_t volume);

 private:
  uint8_t current_mode_{0};
  uint8_t volume_{128};
};

}  // namespace radio_controller
}  // namespace esphome


