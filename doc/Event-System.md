# Event System

Unified publish-subscribe event bus for firmware components, controllers, and Home Assistant bridge integration.

## Architecture

**EventBus**: Static allocation, O(n) dispatch (n≤8 subscribers per event), singleton via `eventBus()`

**Event Structure**:
```cpp
namespace events {
struct Event {
  EventType type;           // Enum for type-safe handling
  uint16_t type_id;         // Numeric ID for serialization
  const char* type_name;    // String name (auto-populated from catalog)
  uint32_t timestamp;       // Milliseconds since boot
  std::string value;        // JSON payload
};
}
```

**Key Files**:
- `include/Events.h` - EventType enum, EventBus interface
- `src/Events.cpp` - Event catalog, EventBus implementation
- `include/events/JsonHelpers.h` - JSON payload builders

---

## Event Catalog

### Current Events

| ID | Name | Enum | Consumers | Payload | Description |
|----|------|------|-----------|---------|-------------|
| 0  | `settings.brightness` | BrightnessChanged | ESPHome | `value` (0-255) | Display brightness |
| 1  | `announcement.requested` | AnnouncementRequested | Display | `text`, `priority?` | Show message |
| 2  | `announcement.completed` | AnnouncementCompleted | Modules | empty | Message done |
| 3  | `system.mode` | ModeChanged | ESPHome | `value`, `name?`, `preset?` | Active mode/preset |
| 4  | `settings.volume` | VolumeChanged | ESPHome | `value` (0-255) | Volume level |

**Architecture**: 
- **Hardware inputs** (button presses, encoder turns): Managed by `InputManager`, used locally by application
- **Device state** (preset selection, mode, brightness, volume): Published to ESPHome for automations
- **Internal communication** (announcements): Event bus within firmware

**Example**: When preset button 3 is pressed, `InputManager` detects the press, `PresetManager` handles the logic and updates state, then publishes `ModeChanged` with preset info. Home Assistant sees the preset selection, not the button press.

### Planned Events

| Name | Payload | Purpose |
|------|---------|---------|
| `display.message` | `text`, `priority`, `timeout_ms` | High-level display text |
| `display.vu` | `left`, `right`, `led_left`, `led_right` | VU meter levels |
| `playback.metadata` | `title`, `artist`, `album`, `codec` | Now playing info |
| `playback.station` | `station_id`, `url` | Station selection |
| `menu.items` | `items[]`, `selected` | Menu content from HA |
| `menu.navigate` | `direction`, `position` | Menu navigation |
| `input.source_selector` | `position`, `source` | 4-way selector |
| `bridge.command` | `type`, `data` | Raw HA command |

---

## JSON Payloads

### Conventions

- Use snake_case keys (`preset_id`, `timeout_ms`)
- Simple values: `{"value":180}`
- Include units in keys (`bitrate_kbps`)
- Keep flat (max 2 levels deep)
- Omit null/empty values

### Helpers

```cpp
using namespace events::json;

// Build payloads
evt.value = object({
    number_field("value", 180),
    string_field("name", "radio"),
    boolean_field("enabled", true)
});

// Conditional fields (skip if false)
evt.value = object({
    number_field("value", mode),
    string_field("name", name, name && name[0]),  // Only if non-empty
    number_field("preset", preset, preset >= 0)    // Only if >= 0
});
```

**API**:
- `number_field(key, value, enabled=true)` - integers/floats
- `string_field(key, value, enabled=true)` - auto-escaping
- `boolean_field(key, value, enabled=true)` - true/false
- `object({...})` - build JSON object
- `escape(str)` - escape special chars

---

## Usage

### Publishing Events

```cpp
void RadioHardware::handlePresetKeyEvent(int row, int col, bool pressed) {
  if (!event_bus_) return;

  events::Event evt(pressed ? EventType::PresetPressed : EventType::PresetReleased);
  evt.timestamp = millis();
  evt.value = events::json::object({
      events::json::number_field("value", col),
      events::json::number_field("aux", row, row != 0)
  });
  event_bus_->publish(evt);
}
```

### Subscribing to Events

```cpp
class PresetManager {
  bool registerEventHandlers() {
    EventBus& bus = eventBus();
    return bus.subscribe(EventType::PresetPressed, &handleEvent, this) &&
           bus.subscribe(EventType::PresetReleased, &handleEvent, this);
  }

  static void handleEvent(const events::Event& event, void* context) {
    auto* self = static_cast<PresetManager*>(context);
    String json = String(event.value.c_str());
    int col = parseIntField(json, "value", -1);
    self->processButton(col);
  }
};
```

### Event Flow

```
Hardware Input → RadioHardware.update()
  → eventBus.publish(preset.pressed)
    → PresetManager.handleEvent() [subscriber]
      → PresetManager.applyAction()
        → eventBus.publish(system.mode)
          → Bridge.publishEvent() [subscriber]
            → ESPHome API → Home Assistant
```

---

## Adding New Events

1. Add enum to `EventType` in `include/Events.h`
2. Add catalog entry in `src/Events.cpp` with unique ID
3. Document payload in this file
4. Update ESPHome component if bridged to HA
5. Add test case in `test/test_native_event_catalog/`

**Example**:
```cpp
// include/Events.h
enum class EventType : uint16_t {
  // ...existing...
  MenuNavigate,
  Count
};

// src/Events.cpp
constexpr EventCatalogEntry kCatalog[] = {
  // ...existing...
  {EventType::MenuNavigate, 9, "menu.navigate"},
};
```

---

## Testing

Run tests: `pio test -e native`

**Test Coverage**:
- `test_native_event_catalog` - Catalog lookups
- `test_native_event_serialization` - Event construction, payloads
- `test_native_json_helpers` - JSON builders, escaping
- `test/bridge/` - Bridge integration

---

## Debugging

### Enable Event Logging

```cpp
void debugLog(const events::Event& evt, void*) {
  Serial.printf("[EVENT] %s: %s\n", evt.type_name, evt.value.c_str());
}

// Log all events
for (int i = 0; i < static_cast<int>(EventType::Count); i++) {
  eventBus().subscribe(static_cast<EventType>(i), debugLog, nullptr);
}
```

### Common Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Events not received | Handler not registered | Check `subscribe()` return value |
| Parse fails | Key mismatch | Verify payload keys match docs |
| Event storm | No debounce | Add rate limiting |

---

## Performance

- **Subscriber limit**: 8 per event type
- **Dispatch**: Synchronous, keep handlers fast (<1ms)
- **Payload size**: Target <200 bytes
- **Memory**: Static allocation only
- **Queuing**: None (events are not buffered)

---

## Integration

Events cross firmware/HA boundary via `HomeAssistantBridge`. See `ESPHome-Integration.md` for details.

**Outbound** (Firmware→HA): Events auto-serialized to ESPHome API  
**Inbound** (HA→Firmware): Bridge converts commands to events

The `RadioControllerComponent` handles bidirectional flow and state sync.