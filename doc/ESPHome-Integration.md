## ESPHome Integration Spec (Draft)

### Goals

- Expose the existing radio firmware to Home Assistant via ESPHome without rewriting hardware logic.
- Keep low-level inputs (presets, encoder, sensors) within firmware, while ESPHome publishes high-level events and accepts commands (modes, playlists, metadata, brightness).
- Provide a flexible bridge (`HomeAssistantBridge`) so new HA automations can drive the display/speaker UX without hard-coding HA logic in C++.
- Follow ESPHome component architecture conventions so the integration is maintainable and compatible with upstream tooling ([Component Architecture](https://developers.esphome.io/architecture/components/)).

The unified event bus and JSON payload conventions are defined in `Event-System.md`. The bridge uses that catalog to map events to Home Assistant entities and services. In standalone/demo builds, `HomeAssistantBridge` can be stubbed to log or inject events via the USB serial console; in ESPHome builds, it binds directly to the ESPHome API (Wi-Fi).

---

### Architecture Overview

1. **Demo Firmware (`main.cpp`)**
   - Standalone ESP32 firmware that runs independently of Home Assistant
   - Uses all the same components (RadioHardware, PresetManager, ClockDisplay, etc.)
   - Uses `StubHomeAssistantBridge` that just logs events for development
   - Fully functional radio without any external dependencies

2. **ESPHome Integration**
   - Same firmware components run as ESPHome external components
   - Uses `ESPHomeAssistantBridge` that integrates with ESPHome's API
   - Exposes HA entities and handles commands via ESPHome
2. **Shared Bridge Library (`include/HomeAssistantBridge.h`, `src/HomeAssistantBridge.cpp`)**
   - Encodes/decodes frames using the event catalog.
   - Provides helpers so firmware controllers can publish events or react to inbound HA commands.
3. **ESPHome Component (`esphome/components/radio_controller/`)**
   - Python module sets up the C++ component, registers triggers, and binds to UART (if used) or direct API callbacks.
   - C++ component extends `Component` and `uart::UARTDevice` (if UART debugging is desired) but primarily uses ESPHome API to forward JSON events.
4. **Device YAML (`esphome/devices/radio.yaml`)**
   - Defines WiFi/API/OTA, entities, and automations that map HA UI to the event catalog.
   - Basic configuration includes essential entities for testing; see Configuration Options below for advanced features.

---

### Event & Payload Mapping

See `Event-System.md` for the full catalog. Every event value is JSON. `HomeAssistantBridge` handles mapping between firmware and HA.

| Event Name            | Direction | JSON Keys (examples)                 | HA Mapping Example                             |
|-----------------------|-----------|--------------------------------------|-----------------------------------------------|
| `input.preset`        | Outbound  | `preset_id`, `label`                 | Expose as `select` or automation trigger.      |
| `settings.brightness` | Outbound  | `value`                              | Update `sensor.radio_brightness`.              |
| `display.message`     | Outbound  | `text`, `priority`, `timeout_ms`     | Update `text_sensor.radio_metadata`.           |
| `playback.metadata`   | Outbound  | `title`, `artist`, `album`, `codec`  | Feed templated text sensor or notify service.  |
| `system.mode`         | Outbound  | `value`                              | Sync `select.radio_mode`.                      |
| `bridge.command`      | Inbound   | differs per command (`type`, `data`) | Raw instructions from HA.                      |
| `settings.volume`     | Inbound   | `value`                              | Trigger volume adjustments (slider).           |

Outbound events = firmware → HA. Inbound commands = HA → firmware (bridge emits events so controllers can react). YAML automations call `from_json()` to inspect payload keys where needed.

Each HA-facing entity or automation subscribes to relevant events:

- `select.radio_mode`: publishes `system.mode` when changed; listens for the same event to stay in sync.
- `number.radio_volume`: publishes `settings.volume` number payloads; listens for canonical updates from firmware (if volume changes locally).
- `text_sensor.radio_metadata`: updates when `display.message` or `playback.metadata` events arrive.
- `switch.radio_station_*`: emits a `bridge.command` JSON payload describing which station to start.

---

### ESPHome Component Structure

- **Directory layout** mirrors ESPHome expectations: `__init__.py`, `homeassistant_bridge.h/.cpp`, optional helper platforms (e.g., `switch.py`).
- **Python module**
  - Declares namespace/class (`homeassistant_bridge_ns.class_("HomeAssistantBridge", cg.Component)`).
  - Provides `CONFIG_SCHEMA` with `cv.GenerateID()` and `CONF_UART_ID` validation.
  - `to_code` registers the component (`cg.register_component`) and associates the UART parent; wires frame callback to forward bridge events to automations.
- **C++ module**
  - Extends `Component` and `uart::UARTDevice`.
  - Implements non-blocking `setup`, `loop`, and `dump_config`.
  - Exposes helpers that accept HA actions (set mode, set preset, set brightness, send metadata) and translate to event catalog entries.

This structure adheres to ESPHome component architecture and advanced topics such as loop control and shutdown behavior.

---

### High-Level Entities (ESPHome)

| Entity Type | Purpose |
|-------------|---------|
| `select.radio_mode` | current firmware mode; writing triggers `system.mode` event. |
| `switch.radio_station_*` | toggles to request specific presets via events. |
| `number.radio_volume` | publishes `settings.volume`; listens for updates. |
| `text_sensor.radio_metadata` | displays `display.message` or metadata JSON. |
| `sensor.radio_brightness` | derived from `settings.brightness` number events. |
| Automation triggers | general `bridge.event` trigger carrying raw type/value for advanced automations. |

(Entities are examples; hardware mapping may evolve.)

---

### Sample HA Automations & Flows

1. **Radio Preset Handoff**
   - Firmware publishes `input.preset = "preset.3"` → bridge triggers HA automation to start streaming → HA replies with `bridge.command` (JSON describing stream) and optional `playback.metadata` events to update display.

2. **Encoder-Driven Playlist Menu**
   - `SettingsController` emits `menu.navigate` events. When playlist selected, controller sends `playback.station` and `display.message` events; bridge mirrors to HA if needed. Preset buttons remapped while module active.

3. **Ambient Mode**
   - HA publishes `settings.brightness = "40"` and `system.mode = "mode.clock"` at night. Firmware modules respond by showing clock and dim lighting. In the morning, HA sends `system.mode = "mode.radio"` and pushes new metadata; controllers resume playback logic.

---

### Menu & Config Strategy

- Firmware owns interaction/UX (debounce, LED states, long-press logic, menu navigation) by way of controllers and modules reacting to events.
- HA supplies dynamic content (playlists, metadata, announcements) via `bridge.command` events or structured JSON payloads.
- YAML focuses on state exposure and automations rather than wiring buttons.
  - Example: `menu.playlists` section in YAML; ESPHome component emits a `bridge.command` JSON payload to the firmware; controller interprets it and updates internal menu state.
  - Additional advanced behaviour can be added via YAML includes (e.g., `!include esphome/devices/menus.yaml`).

---

### Implementation Status: ✅ Complete

**All core integration components are now implemented and working:**

1. ✅ **Event System**: Firmware controllers use the unified catalog from `Event-System.md` with JSON payloads
2. ✅ **Bridge Backends**: Complete implementation with `StubHomeAssistantBridge`, `SerialHomeAssistantBridge`, and `ESPHomeAssistantBridge`
3. ✅ **ESPHome Component**: Full `RadioControllerComponent` with entity mappings and event handling
4. ✅ **Device YAML**: Complete configuration in `esphome/devices/radio.yaml` with all entities and automations
5. ✅ **Build System**: Firmware compiles successfully and is ready for hardware testing

### Next Development Phase

With the integration foundation complete, the next priorities are:
- **Font System**: Multi-font support with `IFont4x6` interface
- **Menu Module**: Dynamic menu system for playlists and settings
- **VU Meter APIs**: Audio visualization features
- **Enhanced Entities**: Additional sensors and diagnostic information

This architecture successfully keeps firmware authoritative while enabling ESPHome/HA to orchestrate higher-level automation through the shared event catalog.

---

## Configuration Options

The basic `radio.yaml` provides essential entities for testing. Additional features can be added as needed:

### Enhanced State Synchronization

For automatic bidirectional sync between firmware and Home Assistant entities, add entity registration in `on_boot`:

```yaml
esphome:
  on_boot:
    then:
      - lambda: |-
          // Register entities with bridge for automatic updates
          id(bridge).set_mode_select(id(radio_mode_select));
          id(bridge).set_volume_number(id(radio_volume));
          id(bridge).set_metadata_text_sensor(id(radio_metadata));
```

### Additional Controls

```yaml
number:
  - platform: template
    name: "Radio Brightness"
    id: radio_brightness
    min_value: 0
    max_value: 255
    step: 1
    initial_value: 128
    set_action:
      - lambda: id(bridge).send_brightness_command((int) x);

switch:
  - platform: template
    name: "Preset 2 – Classical"
    id: preset_2_classical
    turn_on_action:
      - lambda: id(bridge).send_mode_command(0, "radio", 2);
      - delay: 500ms
      - switch.turn_off: preset_2_classical

  - platform: template
    name: "Preset 3 – Rock"
    id: preset_3_rock
    turn_on_action:
      - lambda: id(bridge).send_mode_command(0, "radio", 3);
      - delay: 500ms
      - switch.turn_off: preset_3_rock
```

### Additional Status Sensors

```yaml
text_sensor:
  - platform: template
    name: "Radio Current Mode"
    id: radio_current_mode
    icon: "mdi:radio"

sensor:
  - platform: template
    name: "Radio Current Brightness"
    id: radio_current_brightness
    icon: "mdi:brightness-6"
```

### Manual State Polling (Alternative to Automatic Sync)

If not using automatic entity registration, add manual polling:

```yaml
interval:
  - interval: 100ms
    then:
      - lambda: |-
          // Manual state updates
          static int last_volume = -1;
          int current_volume = id(bridge).get_volume();
          if (current_volume != last_volume) {
            last_volume = current_volume;
            id(radio_current_volume).publish_state(current_volume);
            auto call = id(radio_volume).make_call();
            call.set_value(current_volume);
            call.perform();
          }
```

### Hardware Configuration

For different ESP32 boards or pin configurations:

```yaml
esp32:
  board: esp32dev  # or esp32-s3-devkitc-1, etc.

uart:
  tx_pin: GPIO17  # Adjust for your hardware
  rx_pin: GPIO16
  baud_rate: 115200
```