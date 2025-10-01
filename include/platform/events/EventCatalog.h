#pragma once

#include "platform/events/Events.h"

struct EventCatalogEntry {
  EventType type;
  uint16_t id;
  const char* name;
};

constexpr EventCatalogEntry kEventCatalog[] = {
  {EventType::PresetPressed, 0, "preset.pressed"},
  {EventType::PresetReleased, 1, "preset.released"},
  {EventType::EncoderTurned, 2, "encoder.turned"},
  {EventType::EncoderPressed, 3, "encoder.pressed"},
  {EventType::BrightnessChanged, 4, "settings.brightness"},
  {EventType::AnnouncementRequested, 5, "announcement.requested"},
  {EventType::AnnouncementCompleted, 6, "announcement.completed"},
  {EventType::ModeChanged, 7, "system.mode"},
  {EventType::VolumeChanged, 8, "settings.volume"}
};

inline EventCatalogEntry findCatalogEntry(EventType type) {
  for (const auto& entry : kEventCatalog) {
    if (entry.type == type) {
      return entry;
    }
  }
  return {EventType::Count, static_cast<uint16_t>(EventType::Count), "unknown"};
}

inline EventCatalogEntry findCatalogEntry(uint16_t id) {
  for (const auto& entry : kEventCatalog) {
    if (entry.id == id) {
      return entry;
    }
  }
  return {EventType::Count, id, "unknown"};
}

inline EventCatalogEntry findCatalogEntry(const char* name) {
  if (!name) {
    return {EventType::Count, static_cast<uint16_t>(EventType::Count), "unknown"};
  }
  for (const auto& entry : kEventCatalog) {
    if (strcmp(entry.name, name) == 0) {
      return entry;
    }
  }
  return {EventType::Count, static_cast<uint16_t>(EventType::Count), "unknown"};
}
