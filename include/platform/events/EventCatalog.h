#pragma once

#include "platform/events/Events.h"

// EventCatalogEntry is defined in Events.h

constexpr EventCatalogEntry kEventCatalog[] = {
  {EventType::BrightnessChanged, 0, "settings.brightness"},
  {EventType::AnnouncementRequested, 1, "announcement.requested"},
  {EventType::AnnouncementCompleted, 2, "announcement.completed"},
  {EventType::ModeChanged, 3, "system.mode"},
  {EventType::VolumeChanged, 4, "settings.volume"}
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
