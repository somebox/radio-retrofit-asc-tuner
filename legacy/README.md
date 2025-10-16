# Legacy Native Firmware

This directory contains the original standalone firmware for the Radio Retrofit project. **This firmware is archived for reference only** - the ESPHome implementation is now the primary and recommended approach.

## Why ESPHome is Preferred

The ESPHome implementation offers:
- **Automatic Home Assistant Integration** - No bridge code needed
- **Remote Management** - OTA updates, remote diagnostics
- **Declarative Configuration** - YAML instead of C++ for settings
- **Built-in Features** - Logging, WiFi management, API all included
- **Active Development** - Part of the broader ESPHome ecosystem

## Architecture (For Reference)

The native firmware demonstrates excellent embedded architecture:

### Core Components

- **`InputManager`** (`platform/InputManager.h`) - Unified input handling
  - ButtonControl, EncoderControl, SwitchControl abstractions
  - Quadrature encoder decoding with detent tracking
  - Long press detection built-in
  
- **`PresetManager`** (`hardware/PresetManager.h`) - Button state and mode switching
  - Display mode management (Retro, Modern, Clock, Animation)
  - Preset context handling (Default, Menu)
  - LED brightness control

- **`RadioHardware`** (`hardware/RadioHardware.h`) - Low-level hardware interface
  - TCA8418 keypad integration
  - IS31FL3737 LED driver
  - I2C communication

- **`DisplayManager`** (`display/DisplayManager.h`) - Display rendering
  - Multiple font support (modern, retro, icon)
  - Text scrolling and markup parsing
  - Brightness control

- **Event System** (`platform/events/`) - Loose coupling between components
  - Publish/subscribe pattern
  - JSON payloads for data
  - Event catalog for type safety

### Unit Tests

30 comprehensive tests cover:
- Input control logic
- Event serialization
- JSON helpers
- Preset mapping
- Markup parsing

Run tests:
```bash
cd /Users/foz/src/radio-retrofit-asc-tuner/legacy
pio test -e native
```

## Building Legacy Firmware

If you need to build the legacy firmware:

```bash
cd /Users/foz/src/radio-retrofit-asc-tuner/legacy
pio run -e esp32-wrover
```

## Migration Notes

If migrating from native to ESPHome:
1. All hardware configs are now in `esphome/devices/radio.yaml`
2. Preset management moved to `radio_controller` component
3. LED control automated in `panel_leds` component
4. No need for bridge code - ESPHome handles HA integration
5. Display scrolling is automatic in `retrotext_display`

## Why Archive?

This code represents significant engineering effort and demonstrates solid embedded architecture principles:
- Clean separation of concerns
- Interface-based design
- Comprehensive testing
- Event-driven architecture

It serves as valuable reference for:
- Understanding the hardware at a low level
- Learning embedded C++ patterns
- Reference for encoder/input handling
- Display rendering techniques

## See Also

- `../esphome/` - Current ESPHome implementation
- `../doc/esphome.md` - ESPHome setup guide
- `../doc/Hardware-Architecture.md` - Hardware specifications

