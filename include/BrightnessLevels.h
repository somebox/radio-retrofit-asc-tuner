#pragma once

// Centralized brightness level management
enum BrightnessLevel {
  BRIGHTNESS_LOW = 64,     // 25% brightness for dimmed elements
  BRIGHTNESS_NORMAL = 230, // 90% brightness for normal operation
  BRIGHTNESS_HIGH = 255,   // 100% brightness for active elements
  BRIGHTNESS_INIT = 128    // 50% brightness during initialization
};

// Helper function to get brightness value
static inline uint8_t getBrightness(BrightnessLevel level) {
  return static_cast<uint8_t>(level);
}
