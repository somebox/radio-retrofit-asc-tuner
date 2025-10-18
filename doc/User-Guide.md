# Radio Retrofit - User Guide

## Controls

### Preset Buttons (1-7)
- **Press**: Jump to that preset and start playing
- **Press again**: Stop playback
- **LED**: Lit brightly when playing that preset

### Memory Button (8)
- **Tap**: Enter preset save mode, tap again to exit. When in save mode, pressing a preset button will save the current station to that preset and select it.
- **LED**: Lit when preset save mode is active

### Rotary Encoder
- **Rotate**: Scroll through all Music Assistant playlists (1 click = 1 item). 
- **Press**: Select and play/pause current item

## Display Modes

### Boot Sequence
1. **"CONNECTING..."** - WiFi connecting
2. **"CONNECTED"** - WiFi connected (shows 1 second)
3. If a stream is already active and matches a preset, the preset LED will light up. Otherwise the preset LED will be off and any metadata available will be shown. If no stream is playing the radio will resume playing the last selected preset.

### Normal Operation

**Playing:**
- Shows station/playlist metadata with play icon
- Long text scrolls automatically
- Active preset LED is bright, if the current stream matches a preset (then the memory button LED is lit dimly).
- Animated spinner shows during connection, disappears when the playing state is confirmed.

**Stopped:**
- Shows station name (static, no scroll) with stop icon
- Preset LED is off
- Clock mode activates after 10 seconds of inactivity

**Browse Mode:**
- Shows available presets and playlists from Music Assistant
- When the visible item matches a preset, the preset LED will light up dimly. Any active preset LED stays bright.
- The memory button LED is lit dimly when the active station is not a preset.
- Browse mode auto-dismisses after 5 seconds of inactivity

**Clock Mode:**
- Activates after 10 seconds when stopped
- Shows date (dim) and time (bright)
- Format: `Oct 16  12:34 PM`
- Any user activity dismisses clock mode

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
- Wait 5 seconds (auto-dismiss), OR
- Select an item (auto-dismiss after selection)

## Other Indicators

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
| `WIFI DISCONNECTED` | WiFi connection lost |
| Spinner animation (`/`, `-`, `\`, `|`) | Stream connecting |

