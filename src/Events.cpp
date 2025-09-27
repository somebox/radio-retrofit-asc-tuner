#include "Events.h"

EventBus::EventBus() {
  clear();
}

bool EventBus::subscribe(EventType type, EventCallback callback, void* context) {
  if (!callback) {
    return false;
  }

  std::size_t idx = typeToIndex(type);
  auto& slots = subscribers_[idx];

  for (auto& slot : slots) {
    if (!slot.callback) {
      slot.callback = callback;
      slot.context = context;
      return true;
    }
  }
  return false;
}

bool EventBus::unsubscribe(EventType type, EventCallback callback, void* context) {
  if (!callback) {
    return false;
  }

  std::size_t idx = typeToIndex(type);
  auto& slots = subscribers_[idx];

  for (auto& slot : slots) {
    if (slot.callback == callback && slot.context == context) {
      slot.callback = nullptr;
      slot.context = nullptr;
      return true;
    }
  }
  return false;
}

void EventBus::publish(const Event& event) const {
  std::size_t idx = typeToIndex(event.type);
  const auto& slots = subscribers_[idx];

  for (const auto& slot : slots) {
    if (slot.callback) {
      slot.callback(event, slot.context);
    }
  }
}

void EventBus::clear() {
  for (auto& slots : subscribers_) {
    for (auto& slot : slots) {
      slot.callback = nullptr;
      slot.context = nullptr;
    }
  }
}

std::size_t EventBus::typeToIndex(EventType type) {
  auto idx = static_cast<std::size_t>(type);
  if (idx >= static_cast<std::size_t>(EventType::Count)) {
    idx = static_cast<std::size_t>(EventType::PresetPressed);
  }
  return idx;
}

EventBus& eventBus() {
  static EventBus bus;
  return bus;
}


