# Home Assistant Automations for Playlist Browsing

Add these two automations to your Home Assistant configuration.

## Automation 1: Load Playlists When Entering Playlist Mode

**Settings** → **Automations & Scenes** → **Create Automation** → **Edit in YAML**

```yaml
alias: "Radio - Load Playlists"
description: "Fetch playlists from Music Assistant when entering playlist mode"

triggers:
  - platform: state
    entity_id: sensor.retro_radio_radio_mode
    to: "playlists"

conditions: []

actions:
  # Fetch playlists from Music Assistant
  - service: music_assistant.get_library
    data:
      config_entry_id: !secret music_assistant_instance_id
      media_type: playlist
      limit: 50
      order_by: last_played
    response_variable: playlist_data
  
  # Format as JSON array and send to ESPHome via custom service
  - service: esphome.retro_radio_load_playlists
    data:
      playlist_json: >-
        {%- set ns = namespace(items=[]) -%}
        {%- for item in playlist_data['items'] -%}
          {%- set ns.items = ns.items + [{'name': item.name, 'uri': item.uri}] -%}
        {%- endfor -%}
        {{ ns.items | tojson }}

mode: restart
```

**Add to `secrets.yaml` (if not already present):**
```yaml
music_assistant_instance_id: "4e0bdb6300bf4ee497964741e5bf55a4"
```

## Automation 2: Update Existing Radio Preset Player (Mode-Aware)

Find your existing **"Radio Preset Player"** automation and **replace it** with this updated version:

```yaml
alias: "Radio Preset Player"
description: "Plays radio presets or playlists when selected"

triggers:
  - platform: state
    entity_id: sensor.retro_radio_current_media_id

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
  
  # Play media (mode-aware)
  - choose:
      # Playlist mode
      - conditions:
          - condition: state
            entity_id: sensor.retro_radio_radio_mode
            state: "playlists"
        sequence:
          - action: music_assistant.play_media
            target:
              entity_id: media_player.macstudio_local
            data:
              media_id: "{{ trigger.to_state.state }}"
              media_type: playlist
              enqueue: replace
              radio_mode: false
    
    # Preset mode (default)
    default:
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

## Setup Steps

1. **Flash the updated firmware** to your ESP32:
   ```bash
   esphome upload esphome/devices/radio.yaml
   ```

2. **Update your automation** with the corrected version above
   - **Important**: The service name is `esphome.retro_radio_load_playlists`
   - This bypasses the 255-character text entity limit

3. **Reload automations** in Home Assistant

4. **Test the flow**:
   - Press preset 8 (Memory button)
   - Display should show "BROWSE PLAYLISTS" then "LOADING..."
   - Once playlists load, display shows first playlist name
   - Press encoder button to select and play

## Debugging

Check these if things don't work:

### In ESPHome Logs:
```bash
esphome logs esphome/devices/radio.yaml
```
Look for:
- `[radio_controller] Entering playlist browser mode`
- `[playlist] Received playlist data: [...]`
- `[radio_controller] Loaded N playlists`
- `[radio_controller] Selected playlist: <name> (URI: ...)`

### In Home Assistant:
- **Developer Tools** → **States** → Check:
  - `sensor.retro_radio_radio_mode` (should be "playlists")
  - `sensor.retro_radio_current_media_id` (should contain playlist URI when selected)
- **Developer Tools** → **Services** → Verify:
  - `esphome.retro_radio_load_playlists` service exists after firmware upload

### Common Issues:

1. **"NO PLAYLISTS" shown**
   - Check automation 1 is enabled and triggered
   - Verify Music Assistant instance ID is correct
   - Check that you have playlists in Music Assistant

2. **Playlists don't play**
   - Verify automation 2 is updated with `media_type: playlist`
   - Check that `sensor.retro_radio_radio_mode` is "playlists"
   - Ensure media player entity ID matches your setup

3. **JSON parse error in logs**
   - Check the JSON format sent from HA
   - Verify template in automation 1 is correct

## Next Steps (Future Enhancements)

- Add encoder rotation support for scrolling through playlists
- Add artist/album browsing modes
- Cache playlists for faster access
- Add search/filter capabilities
