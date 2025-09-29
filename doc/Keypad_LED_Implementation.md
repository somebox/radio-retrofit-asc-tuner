# Keypad and LED Implementation

## Hardware Configuration

### Keypad Matrix (TCA8418)

- **I2C Address**: 0x34, **Matrix**: 4x10, **Row 0**: Preset buttons (columns 0-7)

### LED Matrix (IS31FL3737)

- **I2C Address**: 0x55 (ADDR=SCL), **Matrix**: 12x12, **Row 0**: Preset LEDs

## Preset Button Mapping

| Preset | Button | LED | Function |
|--------|--------|-----|----------|
| 0 | Row 0, Col 0 | SW0, CS0 | Modern Mode |
| 1 | Row 0, Col 1 | SW0, CS1 | Retro Mode |
| 2 | Row 0, Col 2 | SW0, CS2 | Clock Mode |
| 3 | Row 0, Col 3 | SW0, CS3 | Animation Mode |
| 4 | Row 0, Col 4 | SW0, CS4 | Preset 4 (skip CS5) |
| 5 | Row 0, Col 5 | SW0, CS5 | Preset 5 |
| 6 | Row 0, Col 6 | SW0, CS6 | Preset 6 |
| 7 | Row 0, Col 7 | SW0, CS7 | Memory |

## Button Behavior

- **Press**: LED lights bright (255), shows preset name and stays visible
- **Hold**: LED stays bright, announcement stays on screen
- **Release**: LED fades to dim (64), announcement shows for 1 second, then mode starts
- **Mode Switch**: Presets 0-3 switch modes after announcement timeout (Modern, Retro, Clock, Animation)

## System Startup - Integrated Progress Sequence

- **Progress Bar**: Preset LEDs start at 0% (no LEDs lit) and show animated progress (0-100%) during initialization
- **No Initial Sequence**: LEDs remain off until progress begins
- **Display Messages**: Each step shows descriptive message on display:
  1. **Display Test** (20%) - Checkerboard test pattern
  2. **WiFi Setup** (40%) - WiFi manager initialization
  3. **WiFi Connecting** (60%) - Connection attempt
  4. **WiFi Connected** (80%) - Successful connection
  5. **Syncing Time** (90%) - NTP time synchronization
  6. **Ready** (100%) - Initialization complete
- **Default Mode**: Modern (preset 0) - text animation starts immediately after completion
- **Display**: Uses Modern font for announcements, Retro font for Retro mode animations
- **Text Alignment**: Static text is left-aligned to character boundaries
- **State Synchronization**: PresetManager and main loop stay synchronized for mode changes

## Preset Behavior

- **Idempotent**: Pressing the same preset multiple times behaves consistently
- **Mode Changes**: Happen after announcement timeout, ensuring smooth transitions
- **State Management**: PresetManager tracks active preset, main loop manages display mode
- **Announcement Control**: Announcements stay active while button held, timeout after release

## Brightness Levels (Centralized)

- **BRIGHTNESS_LOW**: 64 (25%) for dimmed elements
- **BRIGHTNESS_NORMAL**: 230 (90%) for normal operation
- **BRIGHTNESS_HIGH**: 255 (100%) for active elements
- **BRIGHTNESS_INIT**: 128 (50%) during initialization

### Brightness Management

- **DisplayManager**: Manages display brightness with raw `uint8_t` levels via `setBrightnessLevel()`
- **RadioHardware**: Applies preset LED brightness with `setGlobalBrightness()` using the same raw values
- **PresetManager**: Uses raw brightness values for LED animations
- **AnnouncementModule**: Uses full brightness (255) for visibility
