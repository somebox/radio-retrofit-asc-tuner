# Hardware Architecture

## Overview

The system uses an event-driven architecture with hardware abstraction layers separating physical devices from application logic. All hardware interfacing goes through dedicated manager classes that provide consistent APIs regardless of underlying implementation.

## Core Components

### RadioHardware (`hardware/RadioHardware.h`)

**Purpose**: Unified hardware interface for all panel I/O

**Responsibilities**:
- Initialize TCA8418 keypad controller and IS31FL3737 LED driver
- Manage I2C communication and device scanning
- Provide high-level LED control API
- Own InputManager instance for button/encoder/analog state

**Key Design Decision**: Hardware failures are graceful — system continues with reduced functionality if components fail to initialize.

### InputManager (`platform/InputManager.h`)

**Purpose**: State management for all input controls

**Architecture**: Centralized update loop with frame-based state tracking
- `update()` called once per frame
- Synchronizes `previous` and `current` state for edge detection
- Supports buttons, encoders, switches, and analog inputs

**Control Types**:
- **ButtonControl**: Press/release with long-press detection
- **EncoderControl**: Quadrature decoding with detent counting (4-step cycles)
- **SwitchControl**: Multi-position selector state
- **AnalogControl**: Potentiometer with deadzone + time throttling

**State Pattern**: All controls follow the same lifecycle:
1. `update(now)` - Frame start: sync previous ← current
2. Hardware events/polling update `current` state
3. `changed()` returns `true` if `current != previous`

### PresetManager (`hardware/PresetManager.h`)

**Purpose**: Preset button logic and LED animations

**Responsibilities**:
- Button press/hold/release state machine
- LED feedback (bright on press, dim on release)
- Trigger announcements via `AnnouncementModule`
- Mode switching coordination

**Behavior**:
- **Idempotent**: Pressing same preset repeatedly is safe
- **Announcement Hold**: Stays visible while button held
- **Mode Switch**: Occurs after announcement timeout (smooth transitions)

### DisplayManager (`display/DisplayManager.h`)

**Purpose**: Abstract 3-module RetroText display as unified canvas

**Features**:
- 24×6 logical pixel buffer
- Multiple font support via `FontManager`
- Per-board and global brightness control
- Hardware coordinate translation (logical → physical pixels)

## Design Decisions

### Software Quadrature Decoding

**Choice**: Decode encoder via TCA8418 key events (not ESP32 PCNT peripheral)

**Rationale**:
- Low-speed application (menu navigation, not motor control)
- No additional GPIO pins required
- Hardware debouncing via TCA8418
- Consistent with button architecture (event-driven)
- Clean integration with InputManager state machine

**Implementation**: Gray code state machine with 4-step detent counting
```
Clockwise:        00 → 01 → 11 → 10 → 00
Counter-clockwise: 00 → 10 → 11 → 01 → 00
```

**Alternative**: For high-speed applications (>1kHz), wire encoder directly to ESP32 GPIO and use hardware PCNT peripheral.

### Analog Input Noise Reduction

**Dual-layer filtering** for potentiometer:
1. **Deadzone** (50 units out of 4095): Filters electrical noise
2. **Time Throttling** (150ms min interval): Prevents rapid oscillations

**Design**: Poll every frame (~100 Hz), but only accept changes meeting both thresholds. First value bypasses time throttle (zero latency on startup).

### Event System

**Architecture**: Publish-subscribe pattern via `EventBus` (`platform/events/Events.h`)

**Benefits**:
- Decoupled modules (e.g., encoder events → multiple subscribers)
- ESPHome bridge subscribes without modifying core logic
- Easy to add features (new subscribers to existing events)

**Event Types**: Defined in `EventCatalog.h` with JSON payloads for complex data.

## Initialization Sequence

1. **I2C Setup**: Configure bus, scan devices
2. **Display**: Initialize 3× IS31FL3737 drivers
3. **RadioHardware**: Initialize keypad + LEDs
4. **VU Backlights**: Set to dim (64/255) at startup
5. **InputManager**: Register all controls (buttons, encoder, analog)
6. **WiFi + NTP**: Connect and sync time (with LED progress bar)
7. **Feature Modules**: ClockDisplay, MeteorAnimation, AnnouncementModule

**Progress Visualization**: Preset LEDs show 0-100% progress during WiFi/NTP sync.

## State Synchronization

**PresetManager ↔ main.cpp**: Two-way coordination for mode changes
- PresetManager: Tracks active preset, publishes mode change flag
- main.cpp: Reads flag, updates `DisplayMode`, clears flag
- Ensures mode switches happen after announcements complete

**InputManager ↔ RadioHardware**: InputManager owns state, RadioHardware provides access
- `RadioHardware::inputManager()` exposes reference
- Other modules query state via RadioHardware
- Single source of truth for all input state

## Module Boundaries

**Hardware Layer** (`hardware/`):
- HardwareConfig.h: Pin/address definitions (source of truth)
- RadioHardware: Device initialization and LED control
- PresetManager: Button logic and LED feedback

**Platform Layer** (`platform/`):
- InputManager: Input state machines
- EventBus: Publish-subscribe messaging
- HomeAssistantBridge: ESPHome integration

**Display Layer** (`display/`):
- DisplayManager: Display hardware abstraction
- FontManager: Glyph rendering
- SignTextController: Scrolling text animations

**Features Layer** (`features/`):
- AnnouncementModule: Temporary text display
- ClockDisplay: Time display mode
- DiagnosticsMode: Debug/test interface
- MeteorAnimation: Visual effects

## Testing Strategy

**Native Tests** (`test/`):
- Input controls: State machine validation with injection
- Event system: Serialization and routing
- JSON helpers: Parsing and escaping
- Preset mapping: Hardware config validation

**Hardware Tests**:
- I2C device scanning and communication
- Button matrix scanning (interactive)
- LED test patterns (sequential/all-on)
- Encoder direction detection

**Integration**: Actual behavior validated on hardware (change detection, brightness control, encoder accuracy).

