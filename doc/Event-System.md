# Event System Overview

This document describes the unified event bus used across the radio firmware, controllers, and Home Assistant bridge. It establishes common terminology, payload schema, and design guidelines so components, modules, and controllers can interoperate consistently.

## Goals

- Provide a single event structure used for all publishers and subscribers.
- Keep hardware-facing components reusable by isolating behaviour in controllers/modules.
- Make it easy to add new event types and payloads without touching multiple files.
- Ensure the Home Assistant bridge (`HomeAssistantBridge`) has a clear mapping between local and external events.

## Event Structure

All events share this layout:

| Field       | Type          | Description                                                  |
|-------------|---------------|--------------------------------------------------------------|
| `type_id`   | 16-bit number | Stable identifier for the event (assigned in catalog).      |
| `type_name` | string        | Human-readable name (e.g., `preset.selected`).               |
| `timestamp` | uint32 (ms)   | Millisecond clock when the event was published.             |
| `value`     | string (JSON) | JSON payload describing the event-specific data.            |

Events are defined in a single catalog (`EventCatalog` in code) and grouped by feature domains (inputs, playback, display, settings, bridge, etc.). Adding a new event means registering it in the catalog once with its expected JSON keys.

## JSON Payload Conventions

- Payloads are UTF-8 JSON objects.
- Top-level keys use snake_case (`{"value":180}` or `{"station_id":"station.lofi"}`).
- Include only the fields that are meaningful; omit null/empty values.
- For simple numeric or boolean values, wrap them under a `value` key (e.g., `{"value":180}` or `{"value":true}`).
- Identifiers use descriptive strings (`"mode.radio"`, `"preset.2"`).
- Complex data (metadata, menu items, diagnostics) can include nested objects/arrays, but keep depth shallow for easy parsing on the ESP32.
- If payloads need versioning, include `"schema":1` or similar so consumers can branch as needed.

### Examples

| Event Name            | Example Payload                                      | Notes |
|-----------------------|-------------------------------------------------------|-------|
| `settings.brightness` | `{ "value": 180 }`                                   | Simple scalar update. |
| `input.preset`        | `{ "preset_id": "preset.2" }`                      | Identifier plus room for `label`. |
| `playback.metadata`   | `{ "title": "Nightcall", "artist": "Kavinsky", "bitrate_kbps": 1024 }` | Rich metadata from HA. |
| `display.message`     | `{ "text": "Now Playing", "priority": "info" }`   | Display controller formats text. |
| `display.vu`          | `{ "left": 0.65, "right": 0.62 }`                  | Per-channel levels. |
| `menu.items`          | `{ "items": [{"id":"station.lofi","name":"Lofi Beats"}] }` | Menu population. |
| `favorites.save`      | `{ "station_id": "station.lofi" }`                 | Request HA to persist favourites. |

Controllers and downstream consumers should validate required keys and ignore unknown ones so payloads remain extensible.

## Publishing & Subscribing

### Components & Modules

- **Components** (e.g., `RadioHardware`, `DisplayManager`, `PresetManager`, `SignTextController`) publish low-level hardware notifications and react to events that affect the hardware state. They do not encode scenarios (no knowledge of “streaming radio” or “menu active”).
- **Modules** (e.g., `ClockDisplay`, `MeteorAnimation`, `StreamingRadioModule`, `AnnouncementModule`) encapsulate specific features. They subscribe to relevant events and publish higher-level updates (UI content, state transitions).

### Controllers

Controllers coordinate behaviour between components and modules. They:

- Subscribe to input events (buttons, encoder, bridge commands).
- Activate/deactivate modules based on mode/state.
- Publish events representing business logic with JSON payloads.
- Only forward events when the owning module is active, so multiple controllers can co-exist (e.g., settings controller vs. playback controller).

Examples of controllers:

- `SettingsController`: listens to encoder and preset button events when the menu module is active; publishes `settings.brightness = {"value":180}` or `settings.volume = {"value":72}`, which the display manager or audio subsystem consume.
- `RadioController`: responds to preset selection, fetches stream info, publishes `playback.station = {"station_id":"station.lofi"}`, `playback.metadata = {...}`, and `display.message = {...}`, and calls bridge helpers to notify HA.

### Home Assistant Bridge

`HomeAssistantBridge` is the single integration point for Home Assistant:

- **ESPHome build**: the bridge feeds events into ESPHome entities/triggers and converts inbound commands from the ESPHome API to JSON payloads the controllers understand.
- **Demo build**: the bridge can be stubbed to log events or accept simple serial commands, so developers can inspect or inject events without HA running.
- The bridge performs JSON parsing/encoding only; it never contains UI logic or knowledge of how modules render data.

## Event Catalog Organisation

Events are grouped by domain; each entry includes:

- `type_id`
- `