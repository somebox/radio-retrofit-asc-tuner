# Playlist Browser Feature

## Overview

Adds playlist browsing and playback when preset button 8 (Memory) is pressed. Users scroll through Music Assistant playlists using the rotary encoder and select one to play by pressing the encoder button.

## User Flow

1. **Enter Playlist Mode**: Press preset 8 (Memory button)
   - Display shows: "BROWSE PLAYLISTS"
   - Preset LED 8 stays lit
   - Mode enters playlist browser state

2. **Browse Playlists**: Turn rotary encoder
   - Scrolls through available playlists
   - Display shows current playlist name (scrolls if > 18 chars)
   - Wrap around at ends (first ↔ last)

3. **Select Playlist**: Press encoder button
   - Selected playlist begins playing
   - Display transitions to "now playing" metadata
   - Preset LED 8 remains lit

4. **Exit Mode**: Press preset 8 again OR press another preset
   - Returns to normal operation
   - LED switches to new mode/preset

## Data Flow Architecture

### Home Assistant Automation (Recommended)

**Advantages:**
- Reuses existing `current_media_id` sensor and automation pattern
- Simpler state management with single `radio_mode` sensor
- No new automations needed - extend existing one
- LED state is implicit from preset selection
- Leverages existing Home Assistant integration

**Data Flow:**
```
1. User presses preset 8
   → ESPHome sets: sensor.retro_radio_mode = "playlists"
   → ESPHome sets: sensor.retro_radio_current_media_id = ""
   → Preset LED 8 lights up (preset LED 1-7 turn off)
   
2. Home Assistant automation triggers (on mode change to "playlists")
   → Calls music_assistant.get_library with media_type: playlist
   → Formats playlist data as JSON string
   → Sets text.retro_radio_playlist_data with JSON
   
3. ESPHome receives playlist data
   → Parses JSON (using ArduinoJson)
   → Populates browsable list (up to 50 items)
   → Shows first playlist on display
   
4. User scrolls with encoder
   → ESPHome cycles through local playlist cache
   → Updates display with current playlist name
   
5. User presses encoder to select
   → ESPHome sets: sensor.retro_radio_current_media_id = "{playlist_uri}"
   → Existing HA automation triggers (checks radio_mode)
   → If mode == "playlists": calls music_assistant.play_media with media_type: playlist
   → If mode == "presets": calls music_assistant.play_media as before
```

### Mode Switching Logic

**Preset mode (buttons 1-7):**
- `radio_mode = "presets"`
- One of preset LEDs 1-7 is lit
- `current_media_id` contains preset target (e.g., "Radio 3FACH")

**Playlist mode (button 8):**
- `radio_mode = "playlists"`
- Preset LED 8 is lit
- `current_media_id` contains playlist URI when selected (e.g., "spotify://playlist/...")

## ESPHome Component Changes

### New/Modified Sensors

```yaml
# In radio.yaml

text_sensor:
  # NEW: Radio mode selector
  - platform: template
    name: "Radio Mode"
    id: radio_mode
    icon: mdi:radio
    # Values: "presets" or "playlists"
  
  # EXISTING: Current Media ID (already defined)
  # - Used for both preset targets and playlist URIs
  # - Triggers existing HA automation

text:
  # NEW: JSON playlist data from HA
  - platform: template
    name: "Playlist Data"
    id: playlist_data
    optimistic: true
    mode: text
    internal: false  # Exposed to HA for automation
    on_value:
      then:
        - lambda: 'id(controller).load_playlist_data(x);'

number:
  # NEW: Current browse index (for debugging)
  - platform: template
    name: "Browse Index"
    id: browse_index
    optimistic: true
    min_value: 0
    max_value: 100
    step: 1
    internal: true
```

### Radio Controller Component Updates

Add to `esphome/components/radio_controller/`:

**New Methods in `radio_controller.h`:**
```cpp
class RadioController {
  // ... existing methods ...
  
  // Playlist browsing
  void enter_browse_mode(const std::string& mode_type);
  void exit_browse_mode();
  void load_playlist_data(const std::string& json_data);
  void scroll_browse_list(int delta);  // +1 = next, -1 = prev
  void select_current_item();
  
private:
  // Browse state
  bool browse_mode_active_{false};
  std::string browse_mode_type_;
  std::vector<PlaylistItem> browse_items_;
  size_t browse_index_{0};
  
  struct PlaylistItem {
    std::string name;
    std::string uri;
  };
  
  // Encoder handling
  void handle_encoder_rotation_(int delta);
  void handle_encoder_press_();
};
```

**Encoder Integration:**
- Connect to existing encoder button detection (row: 2, column: 1)
- Add encoder rotation event handler
- Route encoder events based on browse_mode_active state

**Implementation Notes:**
- Use ArduinoJson library for JSON parsing (already available in ESPHome)
- Limit playlist cache to ~50 items (memory constraint)
- Display shows "NO PLAYLISTS" if data empty
- Handle JSON parse errors gracefully

## Home Assistant Integration

### Automation 1: Load Playlists

```yaml
alias: "Radio - Load Playlists"
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
      order_by: name
    response_variable: playlist_data
  
  # Format as simple JSON array
  - service: text.set_value
    target:
      entity_id: text.retro_radio_playlist_data
    data:
      value: >
        {%- set items = [] -%}
        {%- for item in playlist_data['items'] -%}
          {%- set items = items + [{'name': item.name, 'uri': item.uri}] -%}
        {%- endfor -%}
        {{ items | tojson }}
mode: restart
```

### Automation 2: Play Media (Updated from existing)

```yaml
alias: "Radio Preset Player"
description: "Plays radio presets or playlists when selected"
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

## Display Behavior

### Browse Mode Display States

1. **Entering Mode** (200ms)
   - "BROWSE PLAYLISTS"
   - Preset LED 8: solid

2. **Loading** (while waiting for HA data)
   - "LOADING... /"  (spinner animation)
   - Preset LED 8: solid

3. **Browsing** (normal operation)
   - Show current playlist name
   - Auto-scroll if name > 18 characters
   - Preset LED 8: solid

4. **No Data**
   - "NO PLAYLISTS"
   - Preset LED 8: solid (user can still exit)

5. **Selection Made**
   - "Loading: {playlist}"  (with spinner)
   - Transition to playing metadata

## LED Control

**LED behavior is implicit from mode:**

**Preset mode (radio_mode = "presets"):**
- One of preset LEDs 1-7 is lit (active preset)
- Preset LED 8 is off or dim

**Playlist mode (radio_mode = "playlists"):**
- Preset LED 8 is lit (active mode)
- Preset LEDs 1-7 are off or dim

**Implementation:** No special LED logic needed - existing preset LED system handles this automatically based on which preset (1-8) is "active".

## Testing Strategy

### Unit Tests
- JSON parsing of playlist data
- Browse index wrapping (first/last)
- Empty playlist handling
- Encoder rotation logic

### Integration Tests
1. Enter browse mode → verify sensor state change
2. HA automation triggers → verify playlist data received
3. Encoder scroll → verify display updates
4. Encoder press → verify URI published
5. HA plays media → verify loading/playing states
6. Exit browse mode → verify state cleanup

### Hardware Tests
1. Press preset 8 → LED lights, display shows "BROWSE PLAYLISTS"
2. Turn encoder → playlists cycle through
3. Long playlist names → auto-scroll works
4. Press encoder → playlist starts playing
5. Press different preset → browse mode exits, LED switches

## Future Enhancements

### Phase 2 Features (Not in Initial Implementation)
- Browse artists (preset 7 + encoder?)
- Browse albums (preset 6 + encoder?)
- Recently played list
- Favorites/starred items
- Search by first letter (hold preset + encoder)
- Pagination for > 50 items

### Performance Optimizations
- Cache playlist data for 5 minutes
- Lazy load on scroll (if > 50 items)
- Progressive display update (show partial names while scrolling)

## Implementation Checklist

- [ ] Update `radio_controller.h` with browse mode methods
- [ ] Implement JSON parsing in `radio_controller.cpp`
- [ ] Add encoder rotation handler
- [ ] Update preset 8 binding to enter browse mode
- [ ] Add browse state sensors to `radio.yaml`
- [ ] Create HA automations (load playlists, play selection)
- [ ] Update LED control for browse mode
- [ ] Add display state transitions
- [ ] Write unit tests for JSON parsing
- [ ] Hardware test with actual Music Assistant instance
- [ ] Update `doc/esphome.md` with usage instructions
- [ ] Update `doc/work-planning.md` with completion status

## Memory Considerations

**Estimated Memory Usage:**
- Playlist cache (50 items × 80 bytes avg): ~4KB
- JSON parsing buffer (ArduinoJson): ~2KB
- Display string buffer: ~200 bytes
- **Total**: ~6KB additional RAM

**ESP32 has 320KB SRAM**, so this is acceptable.

## Error Handling

1. **No Music Assistant connection**: Display "MA NOT AVAILABLE"
2. **JSON parse error**: Log error, display "DATA ERROR"
3. **Empty playlist list**: Display "NO PLAYLISTS"
4. **Encoder events while loading**: Queue or ignore
5. **HA automation timeout**: Display "TIMEOUT" after 10s

## Configuration

**User-configurable options (in HA):**
- Max playlists to fetch (default: 50)
- Sort order (name, recently_added, random)
- Filter by favorite status
- Auto-exit after selection (default: no, stay in browse mode)

**ESPHome compile-time options:**
- ENABLE_PLAYLIST_CACHE (default: true)
- MAX_PLAYLIST_ITEMS (default: 50)
- BROWSE_TIMEOUT_MS (default: 30000)

## References

- [Music Assistant get_library action](https://www.home-assistant.io/integrations/music_assistant#action-music_assistantget_library)
- [Music Assistant play_media action](https://www.home-assistant.io/integrations/music_assistant#action-music_assistantplay_media)
- Current automation: `doc/esphome.md` lines 100-138
- Encoder implementation: `include/platform/InputControls.h` lines 95-144
- Radio controller: `esphome/components/radio_controller/`
