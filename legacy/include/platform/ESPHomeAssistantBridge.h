#pragma once

#ifdef USE_ESPHOME
#include "esphome/core/component.h"
#include "platform/HomeAssistantBridge.h"

/**
 * ESPHome-specific bridge implementation.
 * This is used when the radio firmware runs as an ESPHome external component.
 * It integrates with ESPHome's API to expose entities and handle commands.
 */
class ESPHomeAssistantBridge : public IHomeAssistantBridge {
 public:
  ESPHomeAssistantBridge() = default;

  void begin() override {
    // ESPHome handles initialization
  }
  
  void update() override {
    // ESPHome handles the main loop
  }
  
  void publishEvent(const events::Event& event) override {
    // Forward events to ESPHome entities/triggers
    // This would integrate with the ESPHome component system
    // to update sensors, trigger automations, etc.
    
    // For now, just log - real implementation would use ESPHome API
    ESP_LOGD("radio", "Event: %s (id:%d) = %s", 
             event.type_name, event.type_id, event.value.c_str());
  }
  
  void setHandler(IHomeAssistantCommandHandler* handler) override {
    handler_ = handler;
    // Register with ESPHome to receive commands from HA
  }

 private:
  IHomeAssistantCommandHandler* handler_{nullptr};
};

#endif // USE_ESPHOME
