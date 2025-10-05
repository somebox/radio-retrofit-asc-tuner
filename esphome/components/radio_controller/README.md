# Radio Controller Component

High-level abstraction for managing radio presets with keypad input and display output.

## Features

- Coordinates between keypad and display components
- Single script for all presets with dynamic parameters
- Flexible data passing to Home Assistant services
- Clean, declarative YAML configuration
- Automatic display updates on button press
- Backwards compatible with direct service calls

## Configuration

```yaml
radio_controller:
  keypad_id: keypad          # Required: ID of tca8418_keypad
  display_id: display        # Required: ID of retrotext_display
  service: script.radio_play_preset  # Optional: Default service (default: script.radio_play_preset)
  
  presets:
    - button: {row: 3, column: 3}
      display: "BBC RADIO 4"   # Text shown on display
      target: "bbc_radio_4"    # Passed as 'target' parameter to service
    
    - button: {row: 3, column: 2}
      display: "JAZZ FM"
      target: "jazz_fm"
      # Can add additional data
      data:
        media_type: "radio"
        source: "tunein"
    
    # Override service for specific preset
    - button: {row: 3, column: 1}
      display: "PODCAST"
      service: script.play_podcast
      target: "daily_tech_news"
  
  controls:
    encoder_button: {row: 2, column: 2}
```

## Home Assistant Integration

### Example Script

Create a script in Home Assistant `configuration.yaml`:

```yaml
script:
  radio_play_preset:
    alias: "Play Radio Preset"
    fields:
      target:
        description: "Station identifier"
        example: "bbc_radio_4"
    sequence:
      - service: media_player.play_media
        target:
          entity_id: media_player.living_room_speaker
        data:
          media_content_type: music
          media_content_id: >
            {% set stations = {
              'bbc_radio_4': 'http://stream.live.vc.bbcmedia.co.uk/bbc_radio_fourfm',
              'jazz_fm': 'http://jazz-wr11.sharp-stream.com/jazz.mp3',
              'classic_fm': 'http://media-ice.musicradio.com/ClassicFMMP3'
            } %}
            {{ stations.get(target, '') }}
```

### Advanced Example with Media Browser

```yaml
script:
  radio_play_preset:
    alias: "Play Radio Preset"
    fields:
      target:
        description: "Media content ID"
      media_type:
        description: "Media type"
        default: "music"
      source:
        description: "Media source"
        default: "tunein"
    sequence:
      - service: media_player.play_media
        target:
          entity_id: media_player.living_room_speaker
        data:
          media_content_type: "{{ media_type }}"
          media_content_id: >
            {% if source == 'tunein' %}
              media-source://radio_browser/{{ target }}
            {% else %}
              {{ target }}
            {% endif %}
```

## Configuration Options

### Root Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `keypad_id` | ID | Yes | - | Reference to `tca8418_keypad` component |
| `display_id` | ID | Yes | - | Reference to `retrotext_display` component |
| `service` | string | No | `script.radio_play_preset` | Default service for all presets |
| `presets` | list | No | `[]` | List of preset configurations |
| `controls` | object | No | - | Optional controls configuration |

### Preset Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `button` | object | Yes | - | Button position: `{row: N, column: M}` |
| `display` | string | No | `target` | Text to show on display |
| `target` | string | No | - | Value passed to service as `target` parameter |
| `service` | string | No | Root `service` | Override service for this preset |
| `data` | map | No | `{}` | Additional key-value pairs passed to service |
| `name` | string | No | - | Alias for `display` (backwards compatibility) |

### Controls Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `encoder_button` | object | No | - | Encoder button position: `{row: N, column: M}` |

## Behavior

1. **Button Press**: When a preset button is pressed:
   - Display updates with the `display` text
   - Service is called with `target` and any additional `data`
   - Logs the action at INFO level

2. **Encoder Button**: When encoder button is pressed:
   - Display shows "TUNING"
   - No service call (handled by encoder rotation logic)

3. **Service Call**: The component calls Home Assistant with:
   - Service name (from preset or default)
   - Data map containing:
     - `target`: The preset's target value
     - Any additional key-value pairs from `data`

## Use Cases

### Simple Radio Presets
```yaml
presets:
  - button: {row: 3, column: 3}
    display: "BBC R4"
    target: "bbc_radio_4"
```

### Media Browser Integration
```yaml
presets:
  - button: {row: 3, column: 3}
    display: "BBC R4"
    target: "s96422"  # TuneIn station ID
    data:
      source: "tunein"
```

### Podcast Shortcuts
```yaml
presets:
  - button: {row: 3, column: 3}
    display: "TECH NEWS"
    service: script.play_podcast
    target: "daily_tech_news"
    data:
      episode: "latest"
```

### Mixed Services
```yaml
service: script.radio_play_preset
presets:
  # Uses default service
  - button: {row: 3, column: 3}
    display: "BBC R4"
    target: "bbc_radio_4"
  
  # Override service
  - button: {row: 3, column: 2}
    display: "SPOTIFY"
    service: script.play_spotify_playlist
    target: "37i9dQZF1DXcBWIGoYBM5M"
```

## Dependencies

- `tca8418_keypad`: For button input
- `retrotext_display`: For text display
- Home Assistant API: For service calls

## See Also

- [TCA8418 Keypad Component](../tca8418_keypad/README.md)
- [RetroText Display Component](../retrotext_display/)
