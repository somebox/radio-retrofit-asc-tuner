#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

enum class EventType : uint16_t {
  BrightnessChanged = 0,
  AnnouncementRequested,
  AnnouncementCompleted,
  ModeChanged,
  VolumeChanged,
  Count
};

struct EventCatalogEntry {
  EventType type;
  uint16_t id;
  const char* name;
};

const EventCatalogEntry* eventCatalogEntries();
std::size_t eventCatalogSize();
const EventCatalogEntry& eventCatalogLookup(EventType type);
const EventCatalogEntry& eventCatalogLookup(uint16_t id);
const EventCatalogEntry& eventCatalogLookup(const char* name);

namespace events {

struct Event {
  EventType type{EventType::ModeChanged};
  uint16_t type_id{0};
  const char* type_name{nullptr};
  uint32_t timestamp{0};
  std::string value;  // JSON payload

  Event();
  explicit Event(EventType t);
};

}  // namespace events

using EventCallback = void(*)(const events::Event&, void* context);

class EventBus {
public:
  static constexpr std::size_t kMaxSubscribersPerEvent = 8;

  EventBus();

  bool subscribe(EventType type, EventCallback callback, void* context = nullptr);
  bool unsubscribe(EventType type, EventCallback callback, void* context = nullptr);
  void publish(const events::Event& event) const;
  void clear();

private:
  static constexpr std::size_t kMaxEventTypes = static_cast<std::size_t>(EventType::Count);

  struct SubscriberSlot {
    EventCallback callback{nullptr};
    void* context{nullptr};
  };

  SubscriberSlot subscribers_[kMaxEventTypes][kMaxSubscribersPerEvent];
};

EventBus& eventBus();


