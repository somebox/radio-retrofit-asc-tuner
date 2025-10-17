# Radio Retrofit - Development Planning

## Current Status

**Phase 1 Complete**: Radio controller unified browse system integrated, deprecated code removed, firmware compiling successfully.

**Focus**: ESPHome is the primary and only active implementation.

## Design Goals

### User Experience
- **Unified Browse**: Single list of presets + playlists, no mode confusion
- **Immediate Feedback**: LEDs and display respond instantly to input
- **Intuitive Navigation**: 1 physical click = 1 menu item
- **Auto-Resume**: Remembers and resumes last preset on boot
- **Visual Clarity**: 3-level LED brightness for state feedback

### Architecture Principles
- **Separation of Concerns**: Hardware drivers independent of logic
- **Reusable Components**: Each component usable in other projects
- **Declarative Config**: YAML for configuration, C++ for logic
- **Clean APIs**: Simple method calls between layers

### Technical Standards
- ESPHome component best practices
- Constructor injection over globals
- Non-blocking, millis()-based updates
- Comprehensive logging for debugging

## Completed Milestones

### ✅ Core Components
- `tca8418_keypad` - I2C keyboard matrix controller
- `retrotext_display` - 72×6 LED matrix with auto-scroll
- `radio_controller` - Radio logic with unified browse system
- `panel_leds` - LED control with brightness feedback

### ✅ Radio Controller Refactor
- Unified browse system (presets + playlists)
- Full quadrature encoder decoding (2-step detent tracking)
- Play/stop toggle on encoder button
- Memory button browse toggle
- 3-level LED brightness (bright/dim/off)
- 5-second browse timeout
- Display state management
- Deprecated code removed

### ✅ Firmware Fixes
- Hardware mapping bug (encoder columns corrected)
- Auto-play last preset on boot
- Spinner clearing fallback logic
- Encoder 2-clicks-per-item fix

### ✅ Documentation Archive
- Native firmware moved to `legacy/` directory
- Outdated refactor docs moved to `doc/legacy/`
- Component READMEs created

### ✅ Metadata Display
- C++ implementation complete in `radio_controller`
- Auto-scrolling for long text on 18-char display
- Smart display logic: metadata when playing, station name otherwise
- HA template sensor configuration provided
- Automations refactored with clear configuration sections
- Full setup guide in `esphome/automations/README.md`

### ✅ Preset Storage & Saving
- Flash-based preset storage (ESP32 Preferences API)
- 8 preset slots with persistent memory
- Memory button long-press (2 sec) enters save mode
- Save any favorite to any preset button
- Empty slot handling with user feedback
- Auto-generated preset sensors expose to HA
- Unified browse: 8 presets + all MA favorites

## Active Development

### Firmware Flash & Testing
- Flash updated firmware with new preset storage system
- Test Memory button long-press (2 sec hold)
- Browse favorites with encoder
- Save station to preset slot
- Verify presets persist across power cycles
- Test empty slot warning message

### Home Assistant Setup
- Create template sensor for metadata extraction
- Import 4 automations (media control, playlists, all favorites, config helper)
- Update entity IDs to match your media player
- Trigger load_all_favorites automation to populate browse list
- Test metadata display on device

### User Testing
- Flash updated firmware to device
- Test all controls per User Guide
- Verify encoder scrolling (1 click = 1 item)
- Verify auto-play on boot
- Verify LED brightness states
- Verify metadata display and auto-scroll
- Test save preset workflow
- Report any issues

## Future Enhancements

### YAML Refactor (Optional)
**Goal**: Reduce `radio.yaml` from 394 lines to ~200 lines

**Approach**:
- Create `retrotext_clock` component (65 lines → 5 lines)
- Move spinner to `radio_controller` C++ (26 lines → 0 lines)
- Move stream state to C++ (15 lines → 3 lines)
- Simplify now-playing handler (35 lines → 3 lines)

**Why Optional**: Current YAML works well. Refactor provides modularity but no functional benefit.

**Decision**: Defer unless clock component is needed for other projects.

### Enhanced Diagnostics (Future)
- System health sensors (uptime, memory, I2C status)
- Browse state debugging
- LED state monitoring
- Button/encoder event logging

### VU Meter Integration (Hardware Dependent)
**Goal**: Real-time VU meter animation

**Requirements**:
- Audio level data from Home Assistant
- Smooth LED animation
- Integration with panel LEDs component

**Approach**: Add VU meter APIs to panel_leds, subscribe to HA audio sensors

### Source Selector (Hardware Dependent)
**Goal**: 4-way selector switch with LED indicators

**Requirements**:
- 4-way switch wired to TCA8418
- 4× position indicator LEDs
- Source switching automation in HA

**Approach**: Detect switch state, fire HA event, update position LEDs

## Design Decisions

### Why ESPHome Over Native Firmware?
- **HA Integration**: Automatic entity exposure, no bridge code
- **OTA Updates**: Remote firmware updates
- **Declarative Config**: YAML is easier to modify than C++
- **Logging**: Built-in remote logging via HA
- **Community**: ESPHome has active development and support

**Result**: Native firmware archived to `legacy/` as reference.

### Why Unified Browse Instead of Dual Modes?
- **Simpler**: One list, one state machine
- **More Intuitive**: Users don't think in "preset mode" vs "playlist mode"
- **Easier to Extend**: Adding items doesn't require mode logic
- **Better UX**: Encoder always scrolls through available items

**Result**: Old dual-mode code removed, ~100 lines eliminated.

### Why Full Quadrature Decoding?
- **Reliability**: Single-edge detection missed transitions
- **Accuracy**: 4-step Gray code per detent prevents double-counting
- **Direction Changes**: Proper state tracking handles quick reversals

**Result**: 1 physical click = 1 menu item, direction changes work correctly.

### Why 3-Level LED Brightness?
- **Clarity**: Bright = playing, dim = browsing, off = inactive
- **Feedback**: User instantly sees what's selected vs what's playing
- **UX**: Visual distinction between states

**Result**: LEDs provide clear state feedback without confusion.

## System Architecture

### Hardware
- **MCU**: ESP32 (Arduino framework)
- **Display**: 3× IS31FL3737 (24×6 logical pixels = 18 chars @ 4×6)
- **Panel**: IS31FL3737 LED driver + TCA8418 keypad controller
- **Encoder**: Pushbutton rotary encoder via TCA8418
- **Connectivity**: WiFi + NTP time

### Software Stack
```
┌─────────────────────────────────────┐
│  Home Assistant                     │
│  ├── Music Assistant (playback)    │
│  ├── Automations (preset → play)   │
│  └── Template Sensors (metadata)   │
└────────────┬────────────────────────┘
             │ WiFi/API
┌────────────┴────────────────────────┐
│  ESPHome Firmware (ESP32)           │
│  ├── radio_controller (logic)      │
│  ├── retrotext_display (output)    │
│  ├── tca8418_keypad (input)        │
│  └── panel_leds (feedback)         │
└────────────┬────────────────────────┘
             │ I2C
┌────────────┴────────────────────────┐
│  Hardware                           │
│  ├── RetroText (3× IS31FL3737)     │
│  ├── Panel LEDs (IS31FL3737)       │
│  └── Keypad (TCA8418)              │
└─────────────────────────────────────┘
```

## Known Limitations

1. **Encoder in Preset Mode**: By design, encoder only active in browse mode
2. **First Boot**: Shows "Ready" instead of auto-play (no saved preset)
3. **Clock Component**: Still embedded in YAML (not reusable)

## Next Steps

A. **User Testing** - Flash firmware, test all functionality, report issues
B. **Preset Saving** - If requested by user
C. **YAML Refactor** - If clock component needed elsewhere
D. **Enhanced Diagnostics** - If debugging becomes difficult
