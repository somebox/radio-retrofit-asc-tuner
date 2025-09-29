# Display System Specification

Complete display architecture for LED matrix hardware control, text rendering, fonts, and visual effects.

## Hardware Architecture

### LED Matrix Configuration
- **Hardware**: 3× IS31FL3737 LED drivers (24×6 pixels each)
- **Total Resolution**: 72×6 pixels (18 characters × 4 pixels wide)
- **I2C Addresses**: 0x50 (GND), 0x5A (VCC), 0x5F (SDA)
- **Character Grid**: 4×6 pixel glyphs with configurable spacing

### DisplayManager API
```cpp
DisplayManager(int num_boards = 3, int board_width = 24, int board_height = 6);

// Initialization
bool initialize();
bool verifyDrivers();
void scanI2C();

// Pixel Operations
void setPixel(int x, int y, uint8_t brightness);
void clearBuffer();
void updateDisplay();

// Brightness Control
void setGlobalBrightness(uint8_t brightness);
void setBrightnessLevel(uint8_t value);

// Character Rendering
void drawCharacter(uint8_t pattern[6], int x_offset, uint8_t brightness);
uint8_t getCharacterPattern(uint8_t character, uint8_t row, bool use_alt_font);
```

## Font System

### Current Implementation
- **Modern Font**: Smooth, contemporary 4×6 pixel characters
- **Retro Font**: Pixelated, classic 4×6 pixel characters  
- **Storage**: Character patterns in PROGMEM arrays
- **Selection**: Boolean flag (`use_alt_font`) for font choice

### Character Mapping
- **ASCII Offset**: `character = ascii_char - 32`
- **Pattern Access**: `font_array[3 + character*6 + row] >> 4`
- **Bit Layout**: 4 bits per row, 6 rows per character

## Text Controller System

### SignTextController API
```cpp
namespace RetroText {
    enum Font { MODERN_FONT = 0, ARDUBOY_FONT = 1 };
    enum ScrollStyle { SMOOTH = 0, CHARACTER = 1, STATIC = 2 };
    
    class SignTextController {
        // Configuration
        void setFont(Font font);
        void setScrollStyle(ScrollStyle style);
        void setScrollSpeed(int speed_ms);
        void setBrightness(uint8_t brightness);
        
        // Message Control
        void setMessage(String message);
        void update();
        void reset();
        bool isComplete();
        
        // Text Effects
        void highlightText(int start_char, int end_char, uint8_t brightness);
        void clearHighlights();
    };
}
```

### Current Capabilities
- **Single Font**: One font per controller instance
- **Brightness Spans**: Character-range highlighting with custom brightness
- **Scroll Modes**: Smooth pixel, character-by-character, or static display
- **Integration**: Direct DisplayManager or callback-based rendering

### Current Capabilities
- ✅ **Multi-Font Support**: Mix fonts within one message using FontSpan system
- ✅ **Markup Parsing**: Support for `<f:m>`, `<f:r>`, `<f:i>` font tags and `<b:bright>`, `<b:dim>` brightness tags
- ✅ **FontSpans**: Per-character font control with up to 8 simultaneous font spans
- ✅ **Icon Font**: Music/media symbols available via `ICON_FONT` (♪, ♫, ►, ⏸, etc.)
- ✅ **Brightness Spans**: Character-range brightness control with up to 4 highlight spans

## Brightness Management

### Logical Levels
- **OFF**: 0 - Display off
- **DIM**: 8 - Very dim background
- **LOW**: 30 - Subtle text
- **NORMAL**: 70 - Standard text
- **BRIGHT**: 150 - Emphasized text  
- **MAX**: 255 - Maximum visibility

### Implementation
- **Global Control**: `driver->setGlobalCurrent(brightness)` affects all pixels
- **Per-Pixel**: Individual brightness values (0-255) for each LED
- **Span-Based**: Character ranges can have custom brightness levels

## Display Modes & Integration

### Module Integration
```cpp
// Clock Display
ClockDisplay clock(display_manager, &wifi_time);
clock.update();  // Shows time with brightness-based formatting

// Meteor Animation  
MeteorAnimation meteor(display_manager);
meteor.update();  // Animated effects

// Text Scrolling
SignTextController modern_sign(18, 4);
modern_sign.setFont(RetroText::MODERN_FONT);
modern_sign.setMessage("Scrolling text...");
```

### Event System Integration
- **Brightness Events**: `settings.brightness` with `{"value": 180}` payload
- **Display Messages**: `display.message` with text and priority
- **Mode Changes**: `system.mode` triggers display mode switches

## Multi-Font System Status

### FontSpan System (Implemented)
```cpp
struct FontSpan {
    int start_char;
    int end_char;
    Font font;
    bool active;
};

// Available API
void setFontSpan(int start, int end, Font font);
void clearFontSpans();
Font getActiveFont(int char_index) const;
```

### Markup Support (Implemented)
```cpp
// Inline Tags
sign->setMessageWithMarkup("<f:m>Modern</f> <f:r>retro</f> <f:i>icons</f>");
sign->setMessageWithMarkup("Normal <b:bright>BRIGHT</b> <b:dim>dim</b>");

// Complex Example
sign->setMessageWithMarkup("♪ <f:i>!</f> <b:bright><f:m>Now Playing</f></b>: <f:r>Song Title</f>");

// Nested Tags Supported
sign->setMessageWithMarkup("<f:m>Modern <b:bright>Bright</b> Text</f>");
```

### IFont4x6 Interface (Planned)
```cpp
class IFont4x6 {
public:
    virtual bool getGlyph(uint8_t code, uint8_t outRows[6]) const = 0;
    virtual const char* name() const = 0;
};

class Bitpacked4x6Font : public IFont4x6 { /* Modern/Retro adapters */ };
class Icons4x6Font : public IFont4x6 { /* Icon font */ };
```

## Memory & Performance

### Resource Usage
- **DisplayManager**: ~300 bytes + driver instances
- **SignTextController**: ~200 bytes per instance
- **Font Data**: ~1KB per font in PROGMEM
- **No Dynamic Allocation**: During normal operation

### Performance Characteristics
- **Update Rate**: Configurable scroll speed (40-130ms typical)
- **I2C Speed**: 800kHz for fast display updates
- **Non-blocking**: millis()-based timing, no delays in update loops

## Integration Patterns

### DisplayManager + SignTextController
```cpp
// Direct integration (preferred)
sign->setDisplayManager(display_manager);
sign->update();  // Renders directly to hardware

// Callback integration (legacy)
sign->setRenderCallback(render_char);
sign->setClearCallback(clear_display);
sign->setDrawCallback(update_display);
```

### Multi-Instance Usage
```cpp
// Different fonts for different purposes
auto modern_sign = std::make_unique<SignTextController>(18, 4);
modern_sign->setFont(RetroText::MODERN_FONT);

auto retro_sign = std::make_unique<SignTextController>(18, 4);  
retro_sign->setFont(RetroText::ARDUBOY_FONT);

// Switch between them based on mode
RetroText::SignTextController* active_sign = 
    (mode == MODERN) ? modern_sign.get() : retro_sign.get();
```

## Thread Safety
Not thread-safe. Use appropriate synchronization in multi-threaded environments.
