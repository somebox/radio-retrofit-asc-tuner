### Next steps (short)

1) **Done** – Implemented `EventBus` (`include/Events.h`) and rewired `RadioHardware` preset events to publish/subscribe with `PresetManager` and `AnnouncementModule`.
2) **Done** – Replaced legacy `PresetHandler` with a table-driven `PresetManager` handling buttons 0–8, LED updates, and menu entry/exit scaffolding.
3) **Done** – Simplified brightness handling to raw driver values with shared helpers in `DisplayManager`/`RadioHardware`; brightness events flow through PresetManager actions.
4) Introduce `IFont4x6` and `DisplayManager::drawGlyph4x6`; adapt retro/modern fonts via adapters (no formatting parser yet).
5) Scaffold `MenuModule` (enter via encoder or hold preset; save/exit via Memory), define menu table structure, and persist to EEPROM/NVS.
6) Add VU APIs (`setVuLevels`, `setVuBrightness`) and emit `VuLevelUpdate` events; defer IR feature.
7) Draft ESPHome external-component mapping after refactor (events/state → HA) and prototype local component structure.

---

### System overview

- MCU: ESP32 (Arduino framework)
- Display: 3× IS31FL3737 arranged as 24×6 logical pixels (4×6 glyphs)
- Front panel: 9 buttons (0–8; 0–7 presets, 8=Memory), 9 LEDs
- Connectivity: WiFiManager + NTP time via `WifiTimeLib`
- Roadmap peripherals: dual VU indicators, source selector, rotary encoder


### Patterns to standardize

- Use `std::unique_ptr`, `std::vector`, and `std::array` for ownership.
- Constructor-inject interfaces/callbacks instead of `extern`.
- Centralize enums/consts in headers, use `enum class` and `constexpr`.
- Clamp math, use `size_t` for indices.
- Standardize logging.
- Prefer non-blocking, millis()-based updates.
- Store constant strings in PROGMEM; avoid RAM `String` arrays.
- Single I2C scan utility reused by all modules.


### Specifications and Features

#### Brightness levels (user-configurable, with sensible defaults)

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
  - Remove set-based UI, but instead track global brightness level in `DisplayManager`, and provide a way to adjust it via events.

#### Font handling (multiple fonts, fixed 4x6 glyphs)

- Goal: render text using multiple fonts at once (e.g., retro, modern, and a third icon/special-character font), all with 4x6 glyphs for this PCB.
- Approach: introduce a small font-provider interface `IFont4x6` and decouple the controller from any specific font array.
  - `IFont4x6` (sketch):
    - `bool getGlyph(uint8_t code, uint8_t outRows[6]) const;`  // each row is 4 bits used
    - `const char* name() const;`
  - Adapters:
    - `Bitpacked4x6Font` for existing `retro_font4x6` and `modern_font4x6`.
    - `Icons4x6Font` for the new icon glyph set.
  - `DisplayManager` gets a general `drawGlyph4x6(x, y, rows[6], brightness)` helper; keeps legacy `drawCharacter` for compatibility.

#### Font formatting and spans (enriched text)

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

#### Event pub/sub (decouple modules)

- Goal: decouple `PresetManager`, `AnnouncementModule`, `RadioHardware`, and display modules via a simple event bus.
- Core types (sketch in `include/Events.h`):
  - `enum class EventType : uint8_t { PresetPressed, PresetReleased, ModeChanged, BrightnessChanged, AnnouncementRequested, VolumeChanged, Alarm, ... }`
  - `struct Event { EventType type; uint32_t t; int32_t i1; int32_t i2; const char* s; };`  // small fixed payload
  - `using EventCallback = void(*)(const Event&);`
  - `class EventBus { subscribe(EventType, EventCallback); publish(const Event&); }`
- Flow example:
  - Keypress → `RadioHardware` publishes `PresetPressed{index}`.
  - `PresetManager` consumes, updates state, publishes `ModeChanged{mode}` and optionally `AnnouncementRequested{"Modern"}`.
  - `AnnouncementModule` consumes `AnnouncementRequested`, displays text and notifies when done (optional event).
  - Display modules (`ClockDisplay`, `MeteorAnimation`) listen for `ModeChanged` and take control when announcements are idle.
- Implementation notes:
  - Keep bus lightweight (fixed-size subscriber table per event type, no dynamic allocation in ISR contexts).
  - `RadioHardware` remains a publisher for input events; it no longer needs to call `PresetManager` directly.

---

### Module architecture and expanded design (new)

#### Modules as mini-apps

- Each module owns its state and exposes:
  - `initialize()`, `activate()`, `deactivate()`, `update()`
  - Optional `onEvent(const Event&)` to react to the bus
- Examples to add:
  - `InfoModule`: renders informational banners, status overlays, notifications
  - `StoryModule`: renders continuously scrolling story/messages (smooth or character)
  - `MenuModule`: UI for settings/config/navigation; integrates rotary encoder when available

#### PresetManager responsibilities (new)

- Scope: front panel with 9 buttons (0–8). Buttons 0–7 are presets; button 8 is the Memory button. 9 corresponding LEDs (one per button).
- Inputs: raw button events (`PresetPressed/Released{index}`), optional long-press; rotary encoder press/turn events (used mainly in Menu context).
- Outputs: LED state updates and higher-level events (`PresetSelected`, `PresetMemoryRequested`, `MenuShortcut`, `ModeShortcut`).
- Behavior:
  - Minimal state: current context/mode, pressed index, active index, optional long-press timer.
  - Context mapping table (data-driven): for each context, map buttons 0–7 to actions; button 8 reserved for memory/save (and in Menu, "Save & Exit").
  - Menu entry: press the encoder OR hold any preset (0–7) to enter Menu; in Menu, press Memory (8) to save settings and exit.
  - LEDs: pressed=bright, active=dim, idle=off, updated each loop (all 9 LEDs).
  - Publishes generic events; does not directly invoke modules. Consumers (e.g., radio/menu/modules) decide how to react.
- Goals:
  - Keep logic simple, table-driven, and stateless beyond pressed/active indices.
  - No cross-module calls; only publish events and drive LEDs.

#### Menu module and settings storage

- Menu item types:
  - Action: runs a module or triggers an operation
  - Setting types: numeric (range), select-from-list, boolean on/off
- Menu data source:
  - Defined in a maintainable config (e.g., constexpr tables or a compact JSON-like header for embedded)
  - Persist settings to EEPROM/NVS on change
- Event-driven UX:
  - Rotary encoder publishes events for rotation and press
  - `MenuModule` consumes and updates selection/value; publishes `SettingChanged`
- Entry/Exit:
  - Enter Menu: press the encoder OR hold any preset (0–7)
  - Save & Exit: press the Memory button (index 8) to save and exit

#### Event bus integration

- Central bus routes: `PresetPressed/Released`, `ModeChanged`, `AnnouncementRequested/Done`, `BrightnessChanged`, `SettingChanged`, `EncoderTurned`, `EncoderPressed`, `VuLevelUpdate`, `SourceSelected`.
- Modules subscribe to what they need; publishers do not call consumers directly.
- The current active module is tracked (e.g., by an `App` orchestrator). Announcements can preempt; control returns when `AnnouncementDone` is published.

#### Hardware roadmap considerations

- VU meters (2 indicators):
  - Provide APIs in hardware layer to set per-channel audio level `setVuLevels(uint8_t left, uint8_t right)` and per-indicator LED brightness `setVuBrightness(uint8_t leftBright, uint8_t rightBright)` (0–255)
  - Publish `VuLevelUpdate{left,right}` periodically or on change; modules visualize/react (overlay or dedicated view)
- Source selector switch:
  - Publish `SourceSelected{inputId}`; `RadioHardware` applies selection
- Rotary encoder:
  - `RadioHardware` publishes `EncoderTurned(delta)` and `EncoderPressed`; modules react per context (notably `MenuModule`)
- Ensure the mode management and module focus logic is compatible with these asynchronous events.

#### ESPHome integration (post-refactor)

- Goal: make firmware usable as ESPHome external components with YAML config
- Mapping:
  - Events → Home Assistant events (e.g., `ModeChanged`, `SettingChanged`, `PresetSelected`)
  - Sensor/state exposure: brightness level, mode, menu selection, VU levels, source selection
  - Controls: services/commands mapped to bus events (e.g., request announcement, set brightness, change mode)
-

