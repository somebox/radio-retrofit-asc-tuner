# RetroText Display Component

ESPHome component for controlling a 72×6 LED matrix display using three IS31FL3737 drivers.

## Features

- 72×6 pixel resolution (18 characters at 4×6)
- Auto-scroll for long text
- UTF-8 character mapping for international text
- Adjustable brightness
- Extended character set with media control icons (play, stop, pause, etc.)

## Character Support

### Standard ASCII
Full ASCII 32-126 range with a 4×6 pixel font.

### International Characters
Accented characters are intelligently mapped to simplified glyphs due to the limited 4×6 resolution:
- **Lowercase accented vowels**: á/à/â/ã/ä/å → a with dot, é/è/ê/ë → e with dot, etc.
- **Uppercase accented letters**: Mapped to plain uppercase (Á→A, É→E, etc.)
- **Special characters**: ñ, ç, ß, æ, œ, ø, ¿, ¡, °

### Media Icons (Glyphs 128-139)
128=▶ Play, 129=⏹ Stop, 130=⏸ Pause, 131=⏭ Next, 132=⏮ Previous, 133=Shuffle, 134=Repeat, 135=♥ Heart, 136=♪ Music, 137=📻 Radio, 138=⏺ Record, 139=↻ Refresh

**Usage:**
```cpp
std::string text;
text += (char)128;  // Play icon
text += " Station Name";
display->set_text(text.c_str());
```

## Configuration Example

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true

retrotext_display:
  id: my_display
  board_addresses:
    - 0x50
    - 0x51  
    - 0x52
  brightness: 128
  scroll_mode: auto  # auto, always, never
  scroll_delay: 300ms
```

## API Reference

### Methods

- `set_text(const char *text)` - Display text with automatic scrolling
- `set_brightness(uint8_t brightness)` - Set brightness (0-255)
- `clear()` - Clear the display

### Scroll Modes

- `SCROLL_AUTO` (0): Scroll only if text > 18 characters
- `SCROLL_ALWAYS` (1): Always scroll
- `SCROLL_NEVER` (2): Never scroll (truncate)

## Hardware

Requires three IS31FL3737 LED driver chips connected via I2C. The component handles the complex coordinate mapping for the RetroText PCB layout automatically.

