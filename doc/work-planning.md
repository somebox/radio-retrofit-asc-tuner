# Next steps (short)

1) **Done** – Implemented `EventBus` (`include/Events.h`) and rewired `RadioHardware` preset events to publish/subscribe with `PresetManager` and `AnnouncementModule`.
2) **Done** – Replaced legacy `PresetHandler` with a table-driven `PresetManager` handling buttons 0–8, LED updates, and menu entry/exit scaffolding.
3) **Done** – Simplified brightness handling to raw driver values with shared helpers in `DisplayManager`/`RadioHardware`; brightness events flow through PresetManager actions.
4) Normalize JSON handling: introduce helpers (e.g. `json::object({{"value", value}})`) so controllers/components build payloads consistently, and ensure `HomeAssistantBridge::publishEvent` serializes the `events::Event` fields (`type_id`, `type_name`, `value`).
5) Split bridge backends: define an interface so both ESPHome-backed and demo/serial implementations share the same surface; remove hard UART dependencies from the ESPHome path and rely on ESPHome triggers/entities instead.
6) Clean up references: replace remaining calls to `publishEvent(type, i1, i2, s)` with the new helper and catalog lookups, remove dead includes (especially from `src/main.cpp`), and purge legacy names like `BridgeController`.
7) Update native tests: move bridge protocol tests to assert JSON payloads (`value` field contents, catalog lookups) and add coverage for `HomeAssistantBridge` serialization and catalog resolution once the helpers land.
8) Resume display refactors: introduce `IFont4x6`/`DisplayManager::drawGlyph4x6`, scaffold `MenuModule`, and implement VU APIs after bridge refactor stabilizes.
9) Collapse `src/main.cpp` to a thin façade (`Arduino.h` + `Application` header) once modules own their includes and setup flow.

---

## System overview

- MCU: ESP32 (Arduino framework)
- Display: 3× IS31FL3737 arranged as 24×6 logical pixels (4×6 glyphs)
- Front panel: 9 buttons (0–8; 0–7 presets, 8=Memory), 9 LEDs
- Connectivity: WiFiManager + NTP time via `WifiTimeLib`
- Roadmap peripherals: dual VU indicators, source selector, rotary encoder


## Patterns to standardize

- Use `std::unique_ptr`, `std::vector`, and `std::array` for ownership.
- Constructor-inject interfaces/callbacks instead of `extern`.
- Centralize enums/consts in headers, use `enum class` and `constexpr`.
- Clamp math, use `size_t` for indices.
- Standardize logging.
- Prefer non-blocking, millis()-based updates.
- Store constant strings in PROGMEM; avoid RAM `String` arrays.
- Single I2C scan utility reused by all modules.
- Keep top-level files light: `main.cpp` should include only framework + orchestration headers, while lower layers own their specific driver/font dependencies.


## Specifications and Features

### Brightness levels (user-configurable, with sensible defaults)

- Logical levels: `OFF`, `DIM`, `LOW`, `NORMAL`, `BRIGHT`, `MAX`.
- Default mapping (PWM/global current value):
  - OFF = 0
  - DIM = 8
  - LOW = 30
  - NORMAL = 70
  - BRIGHT = 150
  - MAX = 255
- Global brightness: a single system-wide level applied to both the display and preset LEDs (already coordinated via `DisplayManager` and `RadioHardware`). This is implemented via `driver->setGlobalCurrent(brightness)` so no scaling is needed.
- Implementation notes:
  - Use shared helpers to convert percentages to raw brightness when needed.
  - Remove set-based UI, but instead track global brightness level in `DisplayManager`, and provide a way to adjust it via events (see `Event-System.md` for JSON payload conventions).

### Font handling (multiple fonts, fixed 4x6 glyphs)

- Goal: render text using multiple fonts at once (e.g., retro, modern, and a third icon/special-character font), all with 4x6 glyphs for this PCB.
- Approach: introduce a small font-provider interface `IFont4x6` and decouple the controller from any specific font array.
  - `IFont4x6` (sketch):
    - `bool getGlyph(uint8_t code, uint8_t outRows[6]) const;`  // each row is 4 bits used
    - `const char* name() const;`
  - Adapters:
    - `Bitpacked4x6Font` for existing `retro_font4x6` and `modern_font4x6`.
    - `Icons4x6Font` for the new icon glyph set.
  - `DisplayManager` gets a general `drawGlyph4x6(x, y, rows[6], brightness)` helper; keeps legacy `drawCharacter` for compatibility.

### Font formatting and spans (enriched text)

- Extend `SignTextController` with spans that control brightness and font per character range:
  - `HighlightSpan` (exists) → keep for brightness.
  - Add `FontSpan { int start, int end; const IFont4x6* font; bool active; }`.
  - Render priority per character: `FontSpan` font if active → default font.
- Easy-to-use formatting syntax (two options, both supported):
  - Lightweight inline tags (human-readable):
    - `<f:m>...</f>` = modern font; `<f:r>...</f>` = retro; `<f:i>...</f>` = icons
    - `<b:dim>...</b>` / `<b:low>` / `<b:norm>` / `<b:bright>` / `<b:max>` for brightness spans
  - Compact control codes (efficient for embedded, inside strings):
    - `\x1B Fm`, `\x1B Fr`, `\x1B Fi` to set font; `\x1B Fb` to restore default
    - `\x1B Bd`, `\x1B Bl`, `\x1B Bn`, `\x1B Bb`, `\x1B Bx` for brightness levels
  - `SignTextController` can accept either: a raw message with tags (minimal parser), or a pre-parsed set of spans via an API.
  - `messages.h` can host pre-formatted strings (using tags or control codes) to avoid runtime span setup where appropriate.

### Event system (controllers + modules)

- The event bus uses a unified event structure and catalog detailed in `Event-System.md`.
- Components publish hardware state and react to simple events (brightness updates, LED changes).
- Controllers own behaviour: they subscribe to input events, activate modules, and publish domain events with agreed payloads.
- Modules (clock, radio, announcements, menus, animations) remain reusable, with no knowledge of bridge or HA specifics.
- Bridge translates between local events and Home Assistant; it does not encode UI behaviour.
- Event groups (inputs, playback, display, settings, bridge) and payload expectations are documented in `Event-System.md`.

