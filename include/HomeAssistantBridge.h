#pragma once

#include <Arduino.h>
#include "Events.h"

class IHomeAssistantCommandHandler {
 public:
  virtual void onSetMode(int mode, const String& mode_name, int preset) = 0;
  virtual void onSetVolume(int volume) = 0;
  virtual void onSetBrightness(int value) = 0;
  virtual void onSetMetadata(const String& text) = 0;
  virtual void onRequestStatus() = 0;
  virtual ~IHomeAssistantCommandHandler() = default;
};

// Abstract bridge interface for different backends
class IHomeAssistantBridge {
 public:
  virtual void begin() = 0;
  virtual void update() = 0;
  virtual void publishEvent(const events::Event& event) = 0;
  virtual void setHandler(IHomeAssistantCommandHandler* handler) = 0;
  virtual ~IHomeAssistantBridge() = default;
};

// Stub implementation for demo mode - just logs events for development
class StubHomeAssistantBridge : public IHomeAssistantBridge {
 public:
  StubHomeAssistantBridge() = default;

  void begin() override {
    Serial.println("HomeAssistantBridge: Stub mode (demo app)");
  }
  
  void update() override {
    // No-op in demo mode
  }
  
  void publishEvent(const events::Event& event) override {
    // Log events for development/debugging
    Serial.printf("HA-Bridge: %s (id:%d) = %s\n", 
                  event.type_name, event.type_id, event.value.c_str());
  }
  
  void setHandler(IHomeAssistantCommandHandler* handler) override {
    // No-op in demo mode - no external commands
  }
};

// Serial/UART implementation for ESPHome integration and development
class SerialHomeAssistantBridge : public IHomeAssistantBridge {
 public:
  explicit SerialHomeAssistantBridge(HardwareSerial& serial, uint32_t baud = 115200);

  void begin() override;
  void update() override;
  void publishEvent(const events::Event& event) override;
  void setHandler(IHomeAssistantCommandHandler* handler) override { handler_ = handler; }

 private:
  void processIncoming();
  void handleCommand(const String& line);

  HardwareSerial& serial_;
  uint32_t baud_;
  String rx_buffer_;
  IHomeAssistantCommandHandler* handler_{nullptr};
};

// Build-time bridge selection
#ifdef USE_ESPHOME
  // ESPHome build: use ESPHome integration bridge
  #include "ESPHomeAssistantBridge.h"
  using HomeAssistantBridge = ESPHomeAssistantBridge;
#elif defined(DEMO_SERIAL_BRIDGE)
  // Development build: use serial bridge for testing
  using HomeAssistantBridge = SerialHomeAssistantBridge;
#else
  // Default demo build: use stub bridge (no external dependencies)
  using HomeAssistantBridge = StubHomeAssistantBridge;
#endif


