# Radio Retrofit - User Guide

## Controls

### Preset Buttons (1-7)
- **Press**: Jump to that preset and start playing
- **Press again**: Stop playback
- **LED**: Lit brightly when playing that preset

### Memory Button (8)
- **Tap**: Toggle browse mode on/off
- **Hold + Preset** (future): Save current station to that preset
- **LED**: Lit when browse mode is active

### Rotary Encoder
- **Rotate**: Scroll through presets and playlists (1 click = 1 item)
- **Press while browsing**: Select and play current item
- **Press while playing**: Stop playback

## Display Modes

### Boot Sequence
1. **"CONNECTING..."** - WiFi connecting
2. **"CONNECTED"** - WiFi connected (shows 1 second)
3. **Auto-play** - Resumes last selected preset

### Normal Operation

**Playing:**
- Shows station/playlist metadata
- Long text scrolls automatically
- Preset LED is bright
- Animated spinner shows during connection

**Stopped:**
- Shows station name (static, no scroll)
- Preset LED is off

**Browse Mode:**
- Shows available presets and playlists
- Selected item LED dims while scrolling
- Playing preset LED stays bright
- Auto-dismisses after 5 seconds of inactivity

**Clock Mode:**
- Activates after 10 seconds when stopped
- Shows date (dim) and time (bright)
- Format: `Oct 16  12:34 PM`

## Browse Mode

**How to Enter:**
- Turn encoder knob, OR
- Press Memory button

**How to Navigate:**
- Turn encoder clockwise: scroll forward
- Turn encoder counter-clockwise: scroll backward
- One physical click = one menu item

**How to Select:**
- Press encoder button to play selected item
- Press preset button to jump directly to that preset

**How to Exit:**
- Press Memory button again, OR
- Wait 5 seconds (auto-dismiss), OR
- Select an item (auto-dismiss after selection)

## LED Behavior

### Normal Playback
- **Active preset**: Bright (255)
- **Other presets**: Off (0)
- **Memory**: Off (unless playlist playing)

### Browse Mode
- **Selected item** (if preset): Medium (128)
- **Playing preset**: Dim (64)
- **Other presets**: Off (0)
- **Memory**: Bright (255)

### VU Meter Backlight
- **Stopped**: 10% brightness
- **Playing**: Fades to 80% over ~2 seconds

### Stereo Mode LED
- **On**: When playing stereo content
- **Off**: When stopped or playing mono

## Status Messages

| Message | Meaning |
|---------|---------|
| `CONNECTING...` | WiFi connecting |
| `CONNECTED` | WiFi connected |
| `STOPPED` | Playback stopped |
| `WIFI DISCONNECTED` | WiFi connection lost |
| Spinner animation (`/`, `-`, `\`, `|`) | Stream connecting |

## Troubleshooting

**Preset button doesn't work:**
- Check Home Assistant automation is active
- Verify `sensor.retro_radio_current_media_id` updates
- Check ESPHome logs for button events

**Encoder requires multiple clicks:**
- This was fixed in firmware - ensure latest version flashed
- Should be exactly 1 physical click = 1 menu item

**Spinner doesn't clear:**
- Check `text.retro_radio_stream_state` updates to "playing"
- Firmware has fallback when metadata arrives

**Display shows wrong order:**
- Verify I2C addresses in configuration match physical boards
- Default: `[0x50, 0x5F, 0x5A]`

**Clock shows wrong time:**
- Verify Home Assistant time sensor is configured
- Check `time_id: ha_time` in configuration

