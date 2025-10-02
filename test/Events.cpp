#include "platform/events/Events.h"

#include <cstring>

namespace {
constexpr EventCatalogEntry kCatalog[] = {
  {EventType::BrightnessChanged, 0, "settings.brightness"},
  {EventType::AnnouncementRequested, 1, "announcement.requested"},
  {EventType::AnnouncementCompleted, 2, "announcement.completed"},
  {EventType::ModeChanged, 3, "system.mode"},
  {EventType::VolumeChanged, 4, "settings.volume"}
};
constexpr std::size_t kCatalogSize = sizeof(kCatalog) / sizeof(kCatalog[0]);
constexpr EventCatalogEntry kUnknownEntry{EventType::Count, static_cast<uint16_t>(EventType::Count), "unknown"};
}

const EventCatalogEntry* eventCatalogEntries() {
  return kCatalog;
}

std::size_t eventCatalogSize() {
  return kCatalogSize;
}

const EventCatalogEntry& eventCatalogLookup(EventType type) {
  for (const auto& entry : kCatalog) {
    if (entry.type == type) {
      return entry;
    }
  }
  return kUnknownEntry;
}

const EventCatalogEntry& eventCatalogLookup(uint16_t id) {
  for (const auto& entry : kCatalog) {
    if (entry.id == id) {
      return entry;
    }
  }
  return kUnknownEntry;
}

const EventCatalogEntry& eventCatalogLookup(const char* name) {
  if (!name) {
    return kUnknownEntry;
  }
  for (const auto& entry : kCatalog) {
    if (std::strcmp(entry.name, name) == 0) {
      return entry;
    }
  }
  return kUnknownEntry;
}

events::Event::Event() {
  auto entry = eventCatalogLookup(type);
  type_id = entry.id;
  type_name = entry.name;
}

events::Event::Event(EventType t) : type(t) {
  auto entry = eventCatalogLookup(type);
  type_id = entry.id;
  type_name = entry.name;
}
