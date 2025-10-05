# ESPHome Radio Components

This document explains how to use the custom ESPHome components for the Retro Radio project.

## Components

1. **`tca8418_keypad`** - I2C keyboard matrix controller
2. **`retrotext_display`** - LED matrix display (72×6 pixels, 18 characters)
3. **`radio_controller`** - High-level preset and control management

## Basic Configuration

```yaml
external_components:
  - source:
      type: local
      path: ../components

# I2C bus
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz

# Keypad
tca8418_keypad:
  id: keypad
  address: 0x34
  rows: 4
  columns: 10

# Display
retrotext_display:
  id: display
  brightness: 180
  boards: [0x50, 0x5F, 0x5A]
  scroll_delay: 300ms  # Time between scroll steps
  scroll_mode: auto    # auto, always, or never

# Radio controller
radio_controller:
  id: controller
  keypad_id: keypad
  display_id: display
  
  presets:
    - button: {row: 3, column: 3}
      display: "Radio 3FACH Luz"
      target: "Radio 3FACH"
    
    - button: {row: 3, column: 2}
      display: "WKCR Columbia U"
      target: "WKCR"
  
  controls:
    encoder_button: {row: 2, column: 1}
```

## Scrolling Text

Text longer than 18 characters automatically scrolls:
- **`scroll_mode: auto`** (default) - Scroll only if text > 18 chars
- **`scroll_mode: always`** - Always scroll
- **`scroll_mode: never`** - Truncate at 18 chars
- **`scroll_delay`** - Speed of scrolling (200ms-500ms recommended)

## Home Assistant Integration

### 1. Create Template Sensor

**Settings** → **Devices & Services** → **Helpers** → **Create Helper** → **Template** → **Template a sensor**

**Name:** `Radio Now Playing`

**State Template:**
```jinja
{% set player = 'media_player.macstudio_local' %}
{% if is_state(player, 'playing') %}
  {% set title = state_attr(player, 'media_title') | default('') %}
  {% set artist = state_attr(player, 'media_artist') | default('') %}
  {% set album = state_attr(player, 'media_album_name') | default('') %}
  
  {% set parts = [] %}
  {% if album %}{% set parts = parts + [album] %}{% endif %}
  {% if title and title != album %}{% set parts = parts + [title] %}{% endif %}
  {% if artist and artist != album and artist != title %}{% set parts = parts + [artist] %}{% endif %}
  
  {{ parts | join(' - ') if parts else 'Playing' }}
{% else %}
  Ready
{% endif %}
```

**Icon:** `mdi:radio`

### 2. Create Automation

**Settings** → **Automations & Scenes** → **Automations** → **Create Automation** → **Edit in YAML**

```yaml
alias: Radio Preset Player
description: Plays radio when preset button is pressed
triggers:
  - entity_id: sensor.retro_radio_current_media_id
    trigger: state
conditions:
  - condition: template
    value_template: "{{ trigger.to_state.state not in ['', 'unknown', 'unavailable'] }}"
actions:
  # Set loading state (shows spinner on display)
  - action: text.set_value
    target:
      entity_id: text.retro_radio_stream_state
    data:
      value: loading
  
  # Play media
  - action: music_assistant.play_media
    target:
      entity_id: media_player.macstudio_local
    data:
      media_id: "{{ trigger.to_state.state }}"
      radio_mode: false
  
  # Wait for stream to connect (up to 10 seconds)
  - wait_template: >
      {{ state_attr('media_player.macstudio_local', 'media_position') | float(0) > 0 }}
    timeout: '00:00:10'
    continue_on_timeout: true
  
  # Set playing state (triggers metadata display)
  - action: text.set_value
    target:
      entity_id: text.retro_radio_stream_state
    data:
      value: playing
mode: restart
```

## User Flow

### Boot Sequence
1. **"CONNECTING..."** - Display initializes
2. **"CONNECTED"** - WiFi connects (shows for 2s)
3. **"Ready"** - Normal operation

### Preset Selection
1. **Press preset button** → Display shows station name (e.g., "WKCR Columbia U")
2. **Automation triggers** → Sets `stream_state` to "loading"
3. **Spinner shows** → Display shows "WKCR Columbia U /" (animates)
4. **Stream connects** → Automation sets `stream_state` to "playing"
5. **Metadata displays** → "BBC WORLD SERVICE - West and Central Africa" (scrolls if long)

### Stop Button
- Press encoder button → Stops playback, display shows "STOPPED"

## ESPHome Entities Created

After uploading firmware, these entities are available:

- `text.retro_radio_stream_state` - Set to "loading", "playing", or "stopped"
- `sensor.retro_radio_current_preset` - Current preset display name
- `sensor.retro_radio_current_media_id` - Current media_id (for automation)
- `select.retro_radio_radio_preset` - Preset selector dropdown
- `binary_sensor.retro_radio_preset_button_*` - Individual button states
- `binary_sensor.retro_radio_encoder_button` - Encoder button state

The `radio_now_playing` sensor is created in Home Assistant (not ESPHome).

## Troubleshooting

### Display shows wrong text order
- Check I2C addresses in `boards:` match your hardware
- Typical addresses: `0x50`, `0x5F`, `0x5A`

### Spinner doesn't show
- Check `text.retro_radio_stream_state` in Developer Tools → States
- Should change: "stopped" → "loading" → "playing"

### Metadata doesn't update
- Verify `sensor.radio_now_playing` exists and updates
- Check entity_id in YAML matches exactly

### WiFi connection issues
- Display shows "CONNECTING..." during boot
- Shows "CONNECTED" briefly when connected
- Shows "WIFI DISCONNECTED" if connection drops

### Scrolling too fast/slow
```yaml
retrotext_display:
  scroll_delay: 500ms  # Slower (default: 300ms)
  scroll_delay: 200ms  # Faster
```

## Component Architecture

See `doc/esphome-component-architecture.md` for development details and component design decisions.
