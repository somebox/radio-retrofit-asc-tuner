# Radio Controller Component

High-level radio control system with preset storage, unified browse mode, and LED feedback.

## Features

- **7 Preset Slots**: Flash-persistent storage for favorite stations/playlists
- **Unified Browse**: Single list of presets + all Music Assistant favorites
- **Memory Button**: Save current station to any preset slot
- **LED Feedback**: 3-level brightness (bright/dim/off) for visual state indication
- **Auto-Scroll Display**: Metadata and station names with automatic scrolling
- **Quadrature Encoder**: Full Gray code decoding for reliable navigation
- **Play/Stop Control**: Encoder button toggles playback

## Configuration

```yaml
radio_controller:
  id: controller
  keypad_id: keypad          # Required: ID of tca8418_keypad
  display_id: display        # Required: ID of retrotext_display
  i2c_id: bus_a             # Required: I2C bus for panel LEDs
  
  presets:
    - button: {row: 3, column: 3}
      display: "BBC RADIO 4"
      target: "radio/bbc_radio_4"
    
    - button: {row: 3, column: 2}
      display: "JAZZ FM"
      target: "radio/jazz_fm"
    
    # Define up to 7 preset buttons (columns: 3,2,1,0,8,7,6)
  
  controls:
    encoder_button: {row: 2, column: 1}    # Encoder pushbutton
    encoder_a: {row: 2, column: 3}         # Encoder channel A
    encoder_b: {row: 2, column: 4}         # Encoder channel B
    memory_button: {row: 3, column: 5}     # Save preset mode toggle
```

## Configuration Options

### Root Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `keypad_id` | ID | Yes | - | Reference to `tca8418_keypad` component |
| `display_id` | ID | Yes | - | Reference to `retrotext_display` component |
| `i2c_id` | ID | Yes | - | I2C bus for panel LEDs (auto-detected at 0x55) |
| `service` | string | No | `""` | (Deprecated) Legacy service call support |
| `presets` | list | No | `[]` | List of preset configurations (max 7) |
| `controls` | object | Yes | - | Controls configuration |

### Preset Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `button` | object | Yes | - | Button position: `{row: N, column: M}` |
| `display` | string | Yes | - | Text to show on display |
| `target` | string | Yes | - | Media ID for Music Assistant |

### Controls Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `encoder_button` | object | Yes | - | Encoder button position |
| `encoder_a` | object | No | - | Encoder channel A (for rotation) |
| `encoder_b` | object | No | - | Encoder channel B (for rotation) |
| `memory_button` | object | Yes | - | Memory/save button position |

## Behavior

### Preset Buttons

**Press:** Load and play that preset
- Updates display with station name + play icon
- Lights up corresponding preset LED (bright)
- Publishes media_id to Home Assistant via text sensor

**Saved Presets:** Stored in ESP32 flash memory, persist across reboots

### Memory Button

**Tap once:** Enter save preset mode
- Display shows "SELECT PRESET (TAP MEMORY TO CANCEL)"
- Memory LED lights up bright
- Press any preset button 1-7 to save current station

**Tap again:** Exit save mode without saving
- Returns to now-playing display
- Memory LED returns to normal state

**LED Indicator:**
- Bright: Save mode active OR browsing
- Dim: Playing a non-preset favorite from Music Assistant
- Off: Playing a preset slot OR stopped

### Encoder Control

**Rotate:** Scroll through unified browse list
- 1 physical click = 1 item
- Browse list shows: 7 preset slots + all Music Assistant favorites
- Automatically enters browse mode on first rotation
- Auto-dismisses after 5 seconds of inactivity

**Press:** Play/stop toggle
- If browsing different station: play that station
- If on current station: toggle play/stop
- If stopped: resume playback

### LED Feedback

**Preset LEDs:**
- **Bright (255)**: Currently playing preset
- **Dim (128)**: Selected in browse mode (not playing)
- **Off**: Inactive

**Memory Button LED:**
- **Bright (255)**: Save mode OR browse mode active
- **Dim (64)**: Playing non-preset favorite
- **Off**: Playing preset OR stopped

**Mode LED (Stereo):**
- On when playing
- Off when stopped

**VU Meter Backlight:**
- 10% when stopped
- Fades to 80% over ~2 seconds when playing

## Home Assistant Integration

The radio controller publishes state via ESPHome text sensors and requires Home Assistant automations for playback control.

### Required Text Sensors

Automatically created by the component:

- `sensor.{device_name}_current_preset`: Current preset display name
- `sensor.{device_name}_current_media_id`: Media ID for automation (triggers playback)
- `sensor.{device_name}_radio_mode`: "presets" or "playlists"

### Required Automations

See [`esphome/automations/`](../../automations/) for complete setup:

1. **media_control.yaml**: Plays/stops media when media_id changes
2. **load_all_favorites.yaml**: Populates browse list from Music Assistant on device boot
3. **now_playing_sensor.yaml**: Extracts metadata from media player

**Device Boot Behavior:**
- On WiFi connect, device fires `esphome.retro_radio_sync_favorites` event
- Home Assistant automation catches event and loads radios + playlists
- Browse list updates with all Music Assistant favorites

### Example Automation

```yaml
alias: Radio - Media Control
triggers:
  - entity_id: sensor.retro_radio_current_media_id
    trigger: state
conditions:
  - condition: template
    value_template: "{{ trigger.to_state.state != trigger.from_state.state }}"
actions:
  - choose:
      # Stop if empty media_id
      - conditions:
          - condition: template
            value_template: "{{ trigger.to_state.state == '' }}"
        sequence:
          - action: media_player.media_stop
            target:
              entity_id: "{{ states('input_select.radio_media_player_entity') }}"
      
      # Play if valid media_id
      - conditions:
          - condition: template
            value_template: "{{ trigger.to_state.state not in ['', 'unknown'] }}"
        sequence:
          - action: music_assistant.play_media
            target:
              entity_id: "{{ states('input_select.radio_media_player_entity') }}"
            data:
              media_id: "{{ trigger.to_state.state }}"
              radio_mode: false
```

## Preset Storage

Presets are stored in ESP32 flash memory using ESPHome Preferences API.

**Storage Format:**
- Media ID (128 chars max)
- Display name (64 chars max)
- Valid flag
- Last played timestamp

**Persistence:** Survives reboots, firmware updates, and power loss

**Management:**
- Save: Press memory button → press preset button
- Clear all: Call `esphome.{device}_clear_saved_presets` service

## Browse System

The unified browse system combines presets and Music Assistant favorites into a single scrollable list.

**Browse List Structure:**
1. 7 preset slots (including empty slots marked "Empty Slot X")
2. Separator: "--- ALL FAVORITES ---"
3. All favorited radios from Music Assistant
4. All favorited playlists from Music Assistant

**Navigation:**
- Encoder rotation scrolls through list
- Currently playing item shown with play/stop icon
- Browsed item shown without icon (unless currently playing)
- Preset LEDs light up dim when scrolling over preset slots

**Auto-Dismiss:** Browse mode exits after 5 seconds of inactivity

## Display Behavior

**Station Name:** Shows station/playlist display name
**Metadata:** Shows title - artist - album when playing (from HA template sensor)
**Icons:**
- ▶ (glyph 128): Playing
- ⏹ (glyph 129): Stopped

**Auto-Scroll:** Long text automatically scrolls on the 18-character display

**Priority:**
1. While browsing: show browsed item name
2. While playing: show metadata (if available) or station name
3. While stopped: show last station name with stop icon
4. After 10 seconds stopped: activate clock mode (handled in YAML)

## Services

### clear_saved_presets

Clears all saved presets from flash and reboots (restores YAML defaults).

```yaml
service: esphome.{device_name}_clear_saved_presets
```

### load_all_favorites

Loads favorites from Music Assistant (called by automation).

```yaml
service: esphome.{device_name}_load_all_favorites
data:
  favorites_json: '[{"name": "...", "uri": "..."}]'
```

## Dependencies

- **tca8418_keypad**: Button and encoder input
- **retrotext_display**: 18-char LED matrix display
- **IS31FL3737** (0x55): Panel LEDs for preset/mode indicators
- **Home Assistant API**: State publishing and service calls
- **Music Assistant**: Media playback and favorites

## Hardware Mapping

**Preset Buttons (Row 3):**
- Columns: 3, 2, 1, 0, 8, 7, 6 (7 presets)
- Column 5: Memory button

**Encoder (Row 2):**
- Column 1: Push button
- Column 3: Channel A
- Column 4: Channel B

**Panel LEDs (IS31FL3737 @ 0x55):**
- Preset LEDs: SW3, CS(3,2,1,0,8,7,6)
- Memory LED: SW3, CS5
- Stereo LED: SW0, CS7
- VU Meter: SW2, CS9/CS10

## See Also

- [TCA8418 Keypad Component](../tca8418_keypad/README.md)
- [RetroText Display Component](../retrotext_display/README.md)
- [Home Assistant Automations](../../automations/README.md)
- [User Guide](../../../doc/User-Guide.md)
