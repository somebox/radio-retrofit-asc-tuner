#pragma once

#include <cstdint>
#include <cstddef>

// Lightweight event definitions for decoupling modules

enum class EventType : uint8_t {
  PresetPressed = 0,
  PresetReleased,
  EncoderTurned,
  EncoderPressed,
  BrightnessChanged,
  AnnouncementRequested,
  AnnouncementCompleted,
  ModeChanged,
  Count
};

struct Event {
  EventType type{EventType::PresetPressed};
  uint32_t timestamp{0};
  int32_t i1{0};
  int32_t i2{0};
  const char* s{nullptr};
  void* data{nullptr};
};

using EventCallback = void(*)(const Event&, void* context);

class EventBus {
public:
  static constexpr std::size_t kMaxSubscribersPerEvent = 8;

  EventBus();

  bool subscribe(EventType type, EventCallback callback, void* context = nullptr);
  bool unsubscribe(EventType type, EventCallback callback, void* context = nullptr);
  void publish(const Event& event) const;
  void clear();

private:
  struct SubscriberSlot {
    EventCallback callback{nullptr};
    void* context{nullptr};
  };

  static std::size_t typeToIndex(EventType type);

  SubscriberSlot subscribers_[static_cast<std::size_t>(EventType::Count)][kMaxSubscribersPerEvent];
};

// Accessor for the global event bus instance
EventBus& eventBus();


