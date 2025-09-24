#pragma once

// IS31FL3737 Global Current Control - 5% steps (0-100% in 5% increments)
// IS31FL3737 has 256 steps (0-255), so each step is approximately 0.39%
// For 5% steps: 5% / 0.39% = 12.8 steps per 5%, so we use 13 steps per 5%
#define BRIGHTNESS_STEPS 21  // 0%, 5%, 10%, ..., 100%

// Brightness levels in IS31FL3737 global current values (0-255)
const uint8_t BRIGHTNESS_VALUES[BRIGHTNESS_STEPS] = {
  0,   // 0% - Off
  13,  // 5%  - Very dim
  25,  // 10% - Dim
  38,  // 15% - Low
  51,  // 20% - Low-medium
  64,  // 25% - Medium-low
  76,  // 30% - Medium-low
  89,  // 35% - Medium
  102, // 40% - Medium
  115, // 45% - Medium-high
  127, // 50% - High
  140, // 55% - High
  153, // 60% - High
  165, // 65% - High
  178, // 70% - Very high
  191, // 75% - Very high
  204, // 80% - Very high
  216, // 85% - Very high
  229, // 90% - Maximum
  242, // 95% - Maximum
  255  // 100% - Full brightness
};

// Brightness level enum for easier management
enum BrightnessLevel {
  BRIGHTNESS_0_PERCENT = 0,
  BRIGHTNESS_5_PERCENT = 1,
  BRIGHTNESS_10_PERCENT = 2,
  BRIGHTNESS_15_PERCENT = 3,
  BRIGHTNESS_20_PERCENT = 4,
  BRIGHTNESS_25_PERCENT = 5,
  BRIGHTNESS_30_PERCENT = 6,
  BRIGHTNESS_35_PERCENT = 7,
  BRIGHTNESS_40_PERCENT = 8,
  BRIGHTNESS_45_PERCENT = 9,
  BRIGHTNESS_50_PERCENT = 10,
  BRIGHTNESS_55_PERCENT = 11,
  BRIGHTNESS_60_PERCENT = 12,
  BRIGHTNESS_65_PERCENT = 13,
  BRIGHTNESS_70_PERCENT = 14,
  BRIGHTNESS_75_PERCENT = 15,
  BRIGHTNESS_80_PERCENT = 16,
  BRIGHTNESS_85_PERCENT = 17,
  BRIGHTNESS_90_PERCENT = 18,
  BRIGHTNESS_95_PERCENT = 19,
  BRIGHTNESS_100_PERCENT = 20
};

// Default brightness level (50%)
#define DEFAULT_BRIGHTNESS BRIGHTNESS_50_PERCENT

// Helper function to get IS31FL3737 global current value
static inline uint8_t getBrightnessValue(BrightnessLevel level) {
  int index = static_cast<int>(level);
  if (index >= 0 && index < BRIGHTNESS_STEPS) {
    return BRIGHTNESS_VALUES[index];
  }
  return BRIGHTNESS_VALUES[DEFAULT_BRIGHTNESS];  // Fallback to 50%
}

// Helper function to get brightness percentage string
static inline String getBrightnessPercentage(BrightnessLevel level) {
  int index = static_cast<int>(level);
  if (index >= 0 && index < BRIGHTNESS_STEPS) {
    int percentage = index * 5;
    return String(percentage) + "%";
  }
  return "50%";  // Fallback
}
