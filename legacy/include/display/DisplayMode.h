#pragma once

#include <stdint.h>

// Central display mode definition used across the project.
// Keep ordering stable to match preset mappings and UX labels.
enum class DisplayMode : uint8_t {
  RETRO = 0,
  MODERN = 1,
  CLOCK = 2,
  ANIMATION = 3
};

// Convert a DisplayMode to a human-readable name.
// Intentionally returns simple C strings for low-overhead logging.
inline const char* to_string(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::RETRO: return "Retro";
    case DisplayMode::MODERN: return "Modern";
    case DisplayMode::CLOCK: return "Clock";
    case DisplayMode::ANIMATION: return "Animation";
  }
  return "Unknown";
}


