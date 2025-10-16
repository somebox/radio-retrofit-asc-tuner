# Radio Retrofit - ASC AS-5000E Internet Streamer

Retrofit of a 1970s ASC AS-5000E digital tuner with ESP32 and internet streaming, retaining original controls and aesthetics.

![Radio Retrofit](./doc/hero.jpg)

## What It Does

- **8 Preset Buttons**: Store and recall internet radio stations or playlists
- **Rotary Encoder**: Browse through available stations
- **LED Matrix Display**: Shows station metadata with automatic scrolling (18 characters)
- **LED Feedback**: Preset button LEDs indicate current selection
- **Home Assistant Integration**: Full control via ESPHome components

The original 7-segment frequency display is replaced with [RetroText](https://github.com/PixelTheater/retrotext) (3× 6-character LED matrix modules). Buttons and LEDs are controlled via [Lights n' Buttons](https://github.com/PixelTheater/lights-and-buttons) PCB featuring IS31FL3737 LED drivers and TCA8418 keypad controller.

**User Guide**: See [`doc/User-Guide.md`](doc/User-Guide.md) for detailed interface documentation.

## Quick Start

### Requirements
- ESP32 development board (tested on ESP32-WROVER)
- ESPHome installed (`pip install esphome`)
- Home Assistant instance with Music Assistant
- Hardware: RetroText modules, Lights-and-Buttons PCB, rotary encoder

### Building & Flashing

```bash
# Compile firmware
cd esphome
esphome compile devices/radio.yaml

# Flash to device (USB connected)
esphome upload devices/radio.yaml

# Monitor logs
esphome logs devices/radio.yaml
```

### Home Assistant Setup

1. **Add device**: ESPHome will auto-discover after first flash
2. **Create template sensor** for metadata (see Configuration section below)
3. **Create automation** to play media when preset changes
4. **Configure presets** in `radio.yaml`

## ESPHome Configuration Reference

### Basic Configuration

```yaml
external_components:
  - source:
      type: local
      path: ../components

i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz

# Keypad controller
tca8418_keypad:
  id: keypad
  address: 0x34
  rows: 4
  columns: 10

# LED matrix display
retrotext_display:
  id: display
  board_addresses:
    - 0x50  # Left board
    - 0x5F  # Middle board
    - 0x5A  # Right board
  brightness: 180
  scroll_mode: auto
  scroll_delay: 300ms

# Panel LEDs
panel_leds:
  id: panel_leds
  address: 0x55

# Radio controller
radio_controller:
  id: controller
  keypad_id: keypad
  display_id: display
  panel_leds_id: panel_leds
  
  presets:
    - button: {row: 3, column: 3}
      name: "BBC RADIO 4"
      media_id: "radio/bbc_radio_4"
    
    - button: {row: 3, column: 2}
      name: "JAZZ FM"
      media_id: "radio/jazz_fm"
  
  controls:
    encoder_button: {row: 2, column: 1}
    encoder_a: {row: 2, column: 2}
    encoder_b: {row: 2, column: 3}
    memory_button: {row: 3, column: 9}
```

### Component Options

#### `retrotext_display`

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | required | Component ID for reference |
| `board_addresses` | list | required | I2C addresses of 3 boards (left to right) |
| `brightness` | int | 128 | Global brightness (0-255) |
| `scroll_mode` | enum | `auto` | `auto`, `always`, `never` |
| `scroll_delay` | time | 300ms | Delay between scroll steps |

**Scroll Modes**:
- `auto`: Scroll only if text > 18 characters
- `always`: Always scroll
- `never`: Truncate at 18 characters

#### `tca8418_keypad`

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | required | Component ID |
| `address` | hex | 0x34 | I2C address |
| `rows` | int | required | Number of rows in matrix |
| `columns` | int | required | Number of columns |

#### `panel_leds`

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | required | Component ID |
| `address` | hex | 0x55 | I2C address of IS31FL3737 |

#### `radio_controller`

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | required | Component ID |
| `keypad_id` | ID | required | Reference to keypad |
| `display_id` | ID | required | Reference to display |
| `panel_leds_id` | ID | required | Reference to panel LEDs |
| `presets` | list | `[]` | Preset configurations |
| `controls` | object | required | Control button mappings |

**Preset Options**:
- `button`: `{row: N, column: M}` - Button position in matrix
- `name`: Display name for station
- `media_id`: Media ID passed to Home Assistant

**Control Options**:
- `encoder_button`: Encoder pushbutton position
- `encoder_a`: Encoder channel A position
- `encoder_b`: Encoder channel B position
- `memory_button`: Browse mode toggle button

### Home Assistant Integration

#### Template Sensor for Metadata

Create in **Settings → Devices & Services → Helpers → Template Sensor**:

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

#### Automation for Preset Playback

Create in **Settings → Automations → Create Automation → Edit in YAML**:

```yaml
alias: Radio Preset Player
triggers:
  - entity_id: sensor.retro_radio_current_media_id
    trigger: state
conditions:
  - condition: template
    value_template: "{{ trigger.to_state.state not in ['', 'unknown', 'unavailable'] }}"
actions:
  # Set loading state
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
  
  # Wait for stream
  - wait_template: >
      {{ state_attr('media_player.macstudio_local', 'media_position') | float(0) > 0 }}
    timeout: '00:00:10'
    continue_on_timeout: true
  
  # Set playing state
  - action: text.set_value
    target:
      entity_id: text.retro_radio_stream_state
    data:
      value: playing
mode: restart
```

## Hardware

- **MCU**: ESP32 (Arduino framework)
- **Display**: 3× RetroText (IS31FL3737 drivers, I2C addresses: 0x50, 0x5A, 0x5F)
- **Panel**: Lights-and-Buttons (IS31FL3737 @ 0x55, TCA8418 @ 0x34)
- **Encoder**: Pushbutton rotary encoder connected to TCA8418
- **Power**: 5V external supply, 3.3V logic

See [`doc/Hardware-Architecture.md`](doc/Hardware-Architecture.md) for wiring details.

## Project Structure

```
/esphome          # ESPHome components and device configs
  /components     # Custom ESPHome components
    /tca8418_keypad       # I2C keypad controller
    /retrotext_display    # LED matrix display driver
    /radio_controller     # Radio preset and control logic
    /panel_leds           # Preset LED control
  /devices
    /radio.yaml   # Main device configuration

/doc              # Documentation
  /User-Guide.md  # Interface and controls
  /work-planning.md  # Development planning and TODO
  /legacy         # Archived outdated documentation

/legacy           # Archived native firmware (reference)
  /include        # Native C++ headers
  /src            # Native C++ implementation
  /test           # Unit tests
```

## Contributing

**Development Workflow**:
1. Modify ESPHome components in `esphome/components/`
2. Test with `esphome compile esphome/devices/radio.yaml`
3. Flash to device and verify functionality
4. Update documentation if adding features
5. Submit pull request

**Testing**:
- Component changes: Test with device hardware
- YAML changes: Compile and verify no errors
- Documentation: Ensure accuracy and clarity

See [`doc/work-planning.md`](doc/work-planning.md) for planned enhancements and design decisions.

## License

See [LICENSE](LICENSE) file.

## References

- [RetroText Display](https://github.com/PixelTheater/retrotext)
- [Lights and Buttons PCB](https://github.com/PixelTheater/lights-and-buttons)
- [ESPHome Documentation](https://esphome.io/)
- [Vintage AS-5000E Product Sheet](https://asc6000.de/downloads/asc-hifi/asc-prospekt_as5000e_5000v_1977.pdf)
