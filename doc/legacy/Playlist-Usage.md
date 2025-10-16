# Playlist Browser - User Guide

## Overview

Press the **Memory button (preset 8)** to browse and play playlists from Music Assistant.

## Controls

### Preset Button 8 (Memory)
- **First press**: Enter playlist browser mode
- **Second press**: Exit and stop playback

### Rotary Encoder
- **Turn**: Scroll through playlist list (in browse mode only)
- **Press**: 
  - While browsing → Select and play current playlist
  - While playing → Return to playlist list

## Behavior Flow

### Entering Playlist Mode

1. Press **preset 8** → Display shows "Playlists" (800ms)
2. → Display shows "LOADING..." 
3. → Home Assistant loads playlists from Music Assistant
4. → Display shows first playlist name
5. → Turn encoder to browse through playlists

### Selecting a Playlist

1. While browsing, turn encoder to desired playlist
2. Press **encoder button** → Playlist begins playing
3. → Display transitions to show track metadata (artist, title, album)
4. → Metadata updates as tracks play

### Returning to Playlist List

While a playlist is playing:
- Press **encoder button** → Returns to playlist list
- Turn encoder to select different playlist
- Press encoder again to play new selection

### Exiting Playlist Mode

Press **preset 8** again:
- → Stops playback
- → Shows "Stopped"
- → Exits playlist mode
- → LEDs turn off

## Technical Details

### Encoder Behavior

- **Detent counting**: 4 quadrature steps = 1 click (smooth, responsive)
- **Only active when browsing**: Encoder does nothing while playlist is playing
- **Wraps around**: Last playlist → First playlist (and vice versa)

### Display States

| State | Display Shows | Encoder Function |
|-------|---------------|------------------|
| Entering | "Playlists" (800ms) | Ignored |
| Loading | "LOADING..." | Ignored |
| Browsing | Playlist name | Scroll list |
| Playing | Track metadata | Press to return to list |
| Exiting | "Stopped" | Ignored |

### LED Behavior

- **Preset LED 8**: Lit while in playlist mode (browsing or playing)
- **All LEDs off**: When exiting playlist mode
- **VU meters**: Fade to 80% when playing, 10% when stopped

### Timeout

After selecting a playlist:
- 10 seconds of inactivity → Stays in playing mode showing metadata
- Encoder button still works to return to list

## Changes from Preset Mode

### Encoder Button

**Old behavior (preset mode):**
- Press encoder → Stop playback

**New behavior:**
- In **preset mode**: Encoder button does nothing
- In **playlist mode**: Encoder button selects/returns to list
- To stop in **preset mode**: Press preset 8 twice (enter then exit)

## Examples

### Quick Playlist Play

1. Press preset 8
2. Wait for list to load
3. Turn encoder to desired playlist
4. Press encoder
5. Enjoy!

### Browse Multiple Playlists

1. Press preset 8 → Enter mode
2. Turn encoder → Browse list
3. Press encoder → Play playlist
4. Press encoder again → Return to list
5. Turn encoder → Select different playlist
6. Press encoder → Play new playlist
7. Press preset 8 → Exit and stop

### Return to Regular Radio

While in playlist mode:
- Press **any preset 1-7** → Exits playlist mode, plays that preset
- Or press **preset 8** → Exits and stops playback

## Troubleshooting

### "NO PLAYLISTS" shown
- Check that Music Assistant has playlists
- Verify automation 1 is running
- Check ESPHome logs for "Loaded N playlists"

### Encoder feels laggy
- Firmware now counts full detents (4 steps = 1 click)
- This is intentional for precise selection

### Playlists auto-playing while scrolling
- Fixed: Scrolling only moves through list
- Playback only starts when encoder button is pressed

### Can't stop playback
- Press preset 8 again to exit and stop
- Or press any preset 1-7 to switch to that station

## Logs to Watch

```bash
esphome logs esphome/devices/radio.yaml
```

Look for:
```
[radio_controller] Entering playlist browser mode
[radio_controller] Loaded 6 playlists
[radio_controller] Encoder detent: CW (next)
[radio_controller] Scrolled to playlist 2/6: ...
[radio_controller] Encoder button: selecting playlist
[radio_controller] Selected playlist: ... (URI: ...)
[radio_controller] Encoder button: returning to playlist list
```
