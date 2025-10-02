# Next Steps

## Current Status: ✅ Multi-Font System & ESPHome Integration Complete

The unified event system, JSON payload handling, bridge architecture, ESPHome integration, and multi-font text rendering system are now fully implemented and working. The firmware compiles successfully with streamlined ESPHome configuration ready for testing.

## Next Development Priorities

### ✅ 1. Multi-Font Text Rendering (COMPLETED)

- **Status**: ✅ Fully implemented and tested
- **Features Delivered**:
  - ✅ **FontSpan Structure**: Up to 8 simultaneous font spans per controller
  - ✅ **Markup Parser**: Full support for `<f:m>`, `<f:r>`, `<f:i>` font tags
  - ✅ **Brightness Spans**: Support for `<b:bright>`, `<b:dim>`, `<b:normal>`, `<b:very_dim>` tags
  - ✅ **Nested Tags**: Complex markup with nested font and brightness spans
  - ✅ **Icon Font**: Music/media symbols (♪, ♫, ►, ⏸, ⏹, ⏪, ⏩, 🔊, 🔇, ★)
  - ✅ **Render Integration**: Character rendering uses active font spans
- **API**: `setMessageWithMarkup()`, `setFontSpan()`, `clearFontSpans()`

### ✅ 2. ESPHome Configuration Streamlining (COMPLETED)

- **Status**: ✅ Fully implemented and tested
- **Features Delivered**:
  - ✅ **Simplified Configuration**: Single `radio.yaml` with essential testing components
  - ✅ **RadioControllerComponent**: Complete C++ ESPHome component with automatic state sync
  - ✅ **Entity Registration**: Automatic bidirectional sync between firmware and Home Assistant
  - ✅ **Documentation**: Comprehensive configuration options documented in ESPHome-Integration.md
  - ✅ **Cleanup**: Removed redundant YAML files (radio-clean.yaml, radio-minimal.yaml)
- **API**: Basic entities for mode selection, volume control, preset switches, and status sensors

### ✅ 3. IFont4x6 Interface (COMPLETED)

- **Status**: ✅ Fully implemented and tested
- **Features Delivered**:
  - ✅ **IFont4x6 Interface**: Abstract font interface with clean API
  - ✅ **Font Adapters**: `Bitpacked4x6Font` (modern/retro), `Icons4x6Font` with PROGMEM support
  - ✅ **FontManager**: Centralized font management with Font enum support
  - ✅ **DisplayManager Update**: Replaced boolean flags with Font enum, maintained backward compatibility
  - ✅ **Glyph Helper**: Added `DisplayManager::drawGlyph4x6(x, y, rows[6], brightness)` method
  - ✅ **Icon Font Integration**: Full ICON_FONT rendering support in DisplayManager
  - ✅ **SignTextController Integration**: Updated to use new font interface directly
- **API**: Clean font interface with `getCharacterPattern()`, `getCharacterGlyph()`, `hasCharacter()`, font management via `FontManager`

### ✅ 4. Rotary Encoder Hardware Integration (COMPLETED)

- **Status**: ✅ Fully implemented and tested  
- **Features Delivered**:
  - ✅ **TCA8418 Integration**: Encoder channels wired to Row 1, Col 1-3
  - ✅ **Software Quadrature Decoding**: Gray code state machine for direction detection

### ✅ 5. InputManager Refactoring (COMPLETED)

- **Status**: ✅ Fully implemented and tested
- **Features Delivered**:
  - ✅ **InputControls**: `ButtonControl`, `EncoderControl`, `SwitchControl` classes
  - ✅ **InputManager**: Centralized polling of TCA8418 keypad hardware
  - ✅ **Quadrature Logic**: Full detent counting (1 physical click = 1 position change)
  - ✅ **RadioHardware Integration**: Owns InputManager, removed obsolete encoder state tracking
  - ✅ **PresetManager Refactoring**: Now queries InputManager directly instead of events
  - ✅ **Unit Tests**: 30 comprehensive tests passing (21 input controls + 9 other)
  - ✅ **Time Abstraction**: Mockable `millis()` for native test environment
  - ✅ **Long Press Support**: Built-in long press detection in ButtonControl
- **Benefits**: 
  - Cleaner separation of concerns, easier testing
  - More intuitive API: `input.button(3).wasJustPressed()`
  - Removed ~50 lines of event subscription/handling code
  - Simplified state management (leverages ButtonControl's built-in timing)
- **Code Removed**:
  - Event handler registration/unregistration in PresetManager
  - Redundant `press_times_` array (now handled by ButtonControl)
  - `handlePresetPressedEvent` and `handlePresetReleasedEvent` static callbacks
  - ✅ **Event Publishing**: `encoder.turned` and `encoder.pressed` events
  - ✅ **State Tracking**: Position counter and button state management
  - ✅ **Debug Logging**: Serial output for encoder events
- **Hardware**: ENC_A (R1C1), ENC_B (R1C2), ENC_BUTTON (R1C3) on TCA8418
- **Documentation**: `doc/Rotary-Encoder.md` with implementation details

### 5. Diagnostics Mode ✅

- **Interactive Console**: Serial command interface for hardware testing ✅
- **LED Control**: Direct control of LEDs (`led <row> <col> <brightness>`, `led test`, `led clear`) ✅
- **Controls Monitoring**: Real-time monitoring of buttons, encoder, and switches (`controls`) ✅
- **Event Monitoring**: Live event stream for debugging (`events`) ✅
- **System Info**: Status, memory, uptime, configuration (`info`, `config`)
- **Log Suppression**: Periodic logs (FPS, status) disabled during active diagnostics
- **Simple Commands**: Streamlined interface with help system
- **Build Flag**: `ENABLE_DIAGNOSTICS` to include/exclude from builds
- **Documentation**: `doc/Diagnostics-Mode.md` with simplified command reference

### 6. Menu Module Implementation (Priority 6)

- **MenuModule**: Navigation system for playlists, settings, diagnostics
- **Encoder Control**: Uses encoder.turned/pressed events for navigation
- **Event integration**: Menu state management via event system
- **HA integration**: Dynamic menu content from Home Assistant
- **Display**: Multi-page menus with scroll indicators

### 7. VU Meter APIs

- **Hardware**: Dual analog VU meters with LED indicators
- **LED Control**: 2× VU meter LEDs + 3× meter drive channels via IS31FL3737
- **Bipolar Support**: Virtual ground channel for bipolar meter operation
- **Events**: `display.vu` with `{"left": 0.65, "right": 0.62, "led_left": true, "led_right": false}` payload
- **Integration**: Audio level data from Home Assistant
- **Animation**: Smooth VU meter display updates with LED status indicators

### 8. Source Selector Integration

- **Hardware**: 4-way selector switch connected to TCA8418 (4 channels)
- **LED Indicators**: 4× position LEDs via IS31FL3737 unused channels
- **Events**: `input.source_selector` with `{"position": 1-4, "source": "radio"|"aux"|"bluetooth"|"usb"}` payload
- **Integration**: Source switching automation via Home Assistant
- **Visual Feedback**: LED indication of current source selection

### 9. Additional ESPHome Features

- **Diagnostics**: System health sensors (uptime, memory, I2C status)
- **Advanced Automations**: Scene-based lighting, time-based modes
- **Configuration**: Runtime configuration via HA entities
- **Logging**: Enhanced debug output via ESPHome logs

---

## System Overview

- **MCU**: ESP32 (Arduino framework)
- **Display**: 3× IS31FL3737 arranged as 24×6 logical pixels (4×6 glyphs)
- **Front panel**: 9 buttons (0–8; 0–7 presets, 8=Memory), 9 LEDs
- **Input controller**: TCA8418 keypad controller
- **Connectivity**: WiFiManager + NTP time via `WifiTimeLib`

### Hardware Extensions

- **Rotary Encoder**: ENC_A, ENC_B, ENC_BUTTON connected to TCA8418 channels
  - Menu navigation and settings adjustment
  - Playlist item selection
- **VU Meters**: Dual analog meters with LED indicators
  - 2× VU meter LEDs connected to IS31FL3737 unused channels
  - 3× additional channels for meter control (including bipolar virtual ground)
- **4-Way Source Selector**: Connected to TCA8418 (4 channels)
  - Sound source selection switch
  - 4× position indicator LEDs connected to IS31FL3737 unused channels

## Development Patterns & Standards

### Code Organization

- Use `std::unique_ptr`, `std::vector`, and `std::array` for ownership
- Constructor-inject interfaces/callbacks instead of `extern`
- Centralize enums/consts in headers, use `enum class` and `constexpr`
- Clamp math, use `size_t` for indices
- Prefer non-blocking, millis()-based updates
- Store constant strings in PROGMEM; avoid RAM `String` arrays
- Keep top-level files light: `main.cpp` includes only framework + orchestration headers

### Event System Usage

- All components use the unified event structure from `Event-System.md`
- JSON payloads follow snake_case conventions: `{"value": 180}`
- Use `events::json::object()` helpers for consistent payload building
- Components publish hardware state, Controllers coordinate behavior
- Modules remain reusable with no HA-specific knowledge
- Bridge translates between local events and Home Assistant

### Brightness Management

- **Logical levels**: `OFF`, `DIM`, `LOW`, `NORMAL`, `BRIGHT`, `MAX`
- **Default mapping**: OFF=0, DIM=8, LOW=30, NORMAL=70, BRIGHT=150, MAX=255
- **Global brightness**: Single system-wide level via `driver->setGlobalCurrent(brightness)`
- **Event-driven**: Brightness changes flow through event system

---

## Architecture Status

### ✅ Completed Components

- **EventBus**: Unified publish/subscribe system with catalog-based events
- **JSON Helpers**: Consistent payload format using `events::json::object()`
- **Bridge Architecture**: Multiple backend support (Stub/Serial/ESPHome)
- **ESPHome Integration**: Complete RadioControllerComponent with automatic state sync
- **ESPHome Configuration**: Streamlined single YAML with comprehensive documentation
- **Multi-Font System**: FontSpan architecture with markup parsing and 3 fonts (modern/retro/icon)
- **IFont4x6 Interface**: Clean extensible font system with abstract interface and adapters
- **Core Components**: RadioHardware, DisplayManager, PresetManager, AnnouncementModule
- **Display Modules**: ClockDisplay, MeteorAnimation, SignTextController
- **Tests**: Comprehensive coverage of event system, JSON handling, and markup parsing

### 🚧 Next Phase Components

- **Menu Module**: Navigation and settings UI with encoder control
- **Rotary Encoder Support**: TCA8418 integration for menu navigation
- **VU Meter APIs**: Analog meter control with LED indicators via IS31FL3737
- **Source Selector**: 4-way switch with LED position indicators
- **Enhanced ESPHome**: Additional sensors and automations

## Recent Accomplishments

### ESPHome Configuration Streamlining ✅

- **Consolidated** three separate YAML files into one streamlined `radio.yaml`
- **Implemented** automatic state synchronization via `RadioControllerComponent`
- **Enhanced** documentation with comprehensive configuration options
- **Simplified** testing setup with essential entities only

### Multi-Font System Completion ✅

- **Delivered** complete FontSpan architecture with markup parsing
- **Created** three fonts: modern, retro, and icon fonts with music symbols
- **Implemented** nested tag support for complex text formatting
- **Integrated** brightness span system for text highlighting

### IFont4x6 Interface Implementation ✅

- **Created** clean abstract font interface with extensible design
- **Implemented** font adapters for bitpacked and icon fonts with PROGMEM support
- **Updated** DisplayManager to use Font enum instead of boolean flags
- **Added** `drawGlyph4x6()` helper method for direct glyph rendering
- **Integrated** FontManager for centralized font access and management
- **Maintained** backward compatibility with legacy boolean font selection

## Hardware Design Decisions

### I/O Channel Allocation

**TCA8418 Keypad Controller:**
- 9× preset buttons (0-8)
- 3× rotary encoder channels (ENC_A, ENC_B, ENC_BUTTON)
- 4× source selector channels (4-way switch)
- **Total**: 16 channels utilized

**IS31FL3737 LED Driver Channels:**
- 24×6 = 144 display matrix channels (3 drivers × 48 channels each)
- 9× preset button LEDs
- 2× VU meter indicator LEDs
- 3× VU meter drive channels (including bipolar virtual ground)
- 4× source selector position LEDs
- **Total**: 162 channels (144 display + 18 additional)

### Event System Extensions

New event types to support hardware extensions:
- `input.encoder`: Rotary encoder state changes
- `input.source_selector`: Source switch position changes
- `display.vu`: VU meter levels and LED states
- `hardware.vu_calibration`: Meter calibration parameters

### Development Implications

- **RadioHardware**: Extend TCA8418 support for encoder and selector
- **DisplayManager**: Add VU meter and selector LED control methods
- **Event Catalog**: Add new hardware event definitions
- **ESPHome Integration**: Expose encoder, VU meters, and source selector as HA entities

The foundation is solid and ready for the next development phase!
