# Home Assistant Automations

This directory contains the required Home Assistant automations and template sensors for the Radio Retrofit project.

## Quick Start

1. **Create the configuration helper** (stores your media player entity ID)
2. **Find your media player entity ID** and set it in the helper
3. **Import automations and template sensor** into Home Assistant
4. **Update ESPHome entity names** if your device name differs from "retro_radio"

## Required Components

### 0. Config Helper: `Radio Media Player Entity` (Create First!)
**File:** [`config_helper.yaml`](config_helper.yaml)

Stores your media player entity ID so you only configure it once. All other components reference this helper.

**Setup:**
- Settings → Devices & Services → Helpers → Create Helper → **Dropdown**
- Add your media player entity IDs as options
- Select your current media player

**Why Dropdown?** Prevents typos, shows available players, better UX than text input.

### 0.5. Template Sensor: `Radio Speaker Volume` (Required for Volume Control)
**File:** [`speaker_volume_sensor.yaml`](speaker_volume_sensor.yaml)

Reads the current volume level from your media player and exposes it to ESPHome.

**Setup:**
- Settings → Devices & Services → Helpers → Create Helper → Template → Template a sensor
- Copy the template from the file

**Note:** This is required for the potentiometer volume control to work.

### 1. Template Sensor: `Radio Now Playing`
**File:** [`now_playing_sensor.yaml`](now_playing_sensor.yaml)

Extracts metadata (title, artist, album) from your media player and sends it to the ESPHome device.

**Setup:**
- Settings → Devices & Services → Helpers → Create Helper → Template → Template a sensor
- Copy the template from the file

### 2. Automation: `Radio - Media Control`
**File:** [`media_control.yaml`](media_control.yaml)

Handles play/stop commands when you press preset buttons or the encoder button.

**Setup:**
- Settings → Automations → Create Automation → Edit in YAML
- Paste the content from the file

### 3. Automation: `Radio - Load All Favorites`
**File:** [`load_all_favorites.yaml`](load_all_favorites.yaml)

Fetches ALL favorited radios and playlists from Music Assistant for browsing and saving to the 7 preset slots.

**Setup:**
- Settings → Automations → Create Automation → Edit in YAML
- Paste the content from the file

## Configuration Guide

### Step 1: Create the Configuration Helper

The configuration helper stores your media player entity ID so you only need to set it once.

**In Home Assistant:**
1. Go to **Settings → Devices & Services → Helpers**
2. Click **Create Helper → Dropdown**
3. Set values:
   - **Name:** `Radio Media Player Entity`
   - **Icon:** `mdi:speaker`
   - **Options:** Add your media player entity ID(s) - one per line
4. Select your current media player from the dropdown
5. Click **Create**

This creates `input_select.radio_media_player_entity` which all automations will reference.

**Why Dropdown?** A select/dropdown helper prevents typos and makes it easy to switch between multiple media players.

### Step 2: Find Your Media Player Entity ID

1. Open **Home Assistant → Developer Tools → States**
2. Search for your media player (e.g., "macstudio")
3. Copy the full entity ID (e.g., `media_player.macstudio_local_3`)
4. Paste this into the helper you just created (or update it later via Settings → Helpers)

### Step 3: Find Your Music Assistant Config Entry ID

1. Open **Settings → Devices & Services → Music Assistant**
2. Click the three dots (⋮) → **Download Diagnostics**
3. Open the downloaded JSON file
4. Find the `"entry_id"` value (e.g., `"01K65E8GMGHZXQTFAH6ZJ7TDGY"`)
5. Update this value in `load_all_favorites.yaml` → `music_assistant_config_id` variable

### Step 4: Update ESPHome Entity Names (if needed)

ESPHome automatically creates entities based on your device name in `radio.yaml`.

**Example:** If your ESPHome device is named `"retro-radio"`, entities will be:
- `sensor.retro_radio_current_media_id`
- `sensor.retro_radio_radio_mode`
- `text.retro_radio_stream_state`
- `esphome.retro_radio_load_playlists` (service)

**If your ESPHome device has a different name**, update the entity IDs marked with `# UPDATE THIS:` in:
- `media_control.yaml` → trigger and text entities
- `load_all_favorites.yaml` → service name

### Benefits of the Helper Approach

✅ **Configure once** - Set your media player ID in one place
✅ **Easy updates** - Change media player without editing YAML
✅ **Less error-prone** - No need to update multiple files
✅ **UI configurable** - Change via Settings → Helpers anytime

## Testing

After importing the automations and template sensor:

1. **Check the template sensor:**
   - Developer Tools → States → Search for `sensor.radio_now_playing`
   - Should show "Ready" when idle, or metadata when playing

2. **Test preset playback:**
   - Press a preset button on the radio
   - Check ESPHome logs for "Stream state changed to: loading"
   - Media player should start playing

3. **Test metadata display:**
   - While playing, the display should show the station/track info
   - Long text should auto-scroll on the 18-character display

4. **Test browse mode:**
   - Press the Memory button to enter Browse Mode
   - Encoder should scroll through all favorites
   - Press encoder to play selected item

## Troubleshooting

### Metadata not showing
- Check that `sensor.radio_now_playing` exists in Home Assistant
- Verify the media player entity ID is correct in `now_playing_sensor.yaml`
- Check ESPHome logs for "now_playing" messages

### Presets not playing
- Verify `media_control.yaml` variables match your entity IDs
- Check Home Assistant automation traces (Settings → Automations → Click automation → Traces)
- Check ESPHome logs for media_id changes

### Favorites not loading
- Verify Music Assistant config entry ID in `load_all_favorites.yaml`
- Check ESPHome logs for "Requested favorites sync" on boot
- Check that Music Assistant integration is properly configured
- Check Home Assistant automation traces for the `esphome.retro_radio_sync_favorites` event
- Manually trigger: Developer Tools → Events → Fire Event → `esphome.retro_radio_sync_favorites`

### Wrong entity names
- Check your ESPHome device name in `radio.yaml` (line 7: `name:`)
- Entity IDs are derived from this name (spaces become underscores, hyphens stay)
- Update all automation files with correct entity names

