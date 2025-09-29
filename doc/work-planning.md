# Next Steps

## Current Status: ✅ Event System & Bridge Refactor Complete

The unified event system, JSON payload handling, bridge architecture, and ESPHome integration are now fully implemented and working. The firmware compiles successfully and is ready for the next phase of development.

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

### 2. IFont4x6 Interface (Priority 2 - Next)
- **Goal**: Extensible font system with clean interface
- **Current Status**: Icon font created but DisplayManager still uses boolean flag
- **Implementation Steps**: 
  1. **IFont4x6 Interface**: Define abstract font interface
  2. **Font Adapters**: `Bitpacked4x6Font` (modern/retro), `Icons4x6Font`
  3. **DisplayManager Update**: Replace boolean with Font enum support
  4. **Glyph Helper**: Add `DisplayManager::drawGlyph4x6(x, y, rows[6], brightness)`
  5. **Icon Font Integration**: Enable ICON_FONT rendering in DisplayManager

### 3. Menu Module Implementation
- **MenuModule**: Navigation system for playlists, settings, diagnostics
- **Event integration**: Menu navigation via encoder/preset events
- **HA integration**: Dynamic menu content from Home Assistant
- **Display**: Multi-page menus with scroll indicators

### 4. VU Meter APIs
- **Hardware**: Dual VU indicators (roadmap peripheral)
- **Events**: `display.vu` with `{"left": 0.65, "right": 0.62}` payload
- **Integration**: Audio level data from Home Assistant
- **Animation**: Smooth VU meter display updates

### 5. Additional ESPHome Features
- **Diagnostics**: System health sensors (uptime, memory, I2C status)
- **Advanced Automations**: Scene-based lighting, time-based modes
- **Configuration**: Runtime configuration via HA entities
- **Logging**: Enhanced debug output via ESPHome logs

---

## System Overview

- **MCU**: ESP32 (Arduino framework)
- **Display**: 3× IS31FL3737 arranged as 24×6 logical pixels (4×6 glyphs)
- **Front panel**: 9 buttons (0–8; 0–7 presets, 8=Memory), 9 LEDs
- **Connectivity**: WiFiManager + NTP time via `WifiTimeLib`
- **Roadmap peripherals**: dual VU indicators, source selector, rotary encoder

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
- **ESPHome Integration**: Complete component with entity mappings and state sync
- **Core Components**: RadioHardware, DisplayManager, PresetManager, AnnouncementModule
- **Display Modules**: ClockDisplay, MeteorAnimation, SignTextController
- **Tests**: Comprehensive coverage of event system and JSON handling

### 🚧 Next Phase Components
- **Font System**: `IFont4x6` interface and multi-font support
- **Menu Module**: Navigation and settings UI
- **VU Meter**: Audio visualization APIs
- **Enhanced ESPHome**: Additional sensors and automations

The foundation is solid and ready for the next development phase!