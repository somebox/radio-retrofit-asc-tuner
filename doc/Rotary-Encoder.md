# Rotary Encoder Implementation

## Hardware Configuration

**Connection**: Encoder connected to TCA8418 keypad controller
- **ENC_A**: Row 1, Column 1
- **ENC_B**: Row 1, Column 2  
- **ENC_BUTTON**: Row 1, Column 3

## Implementation Approach

**Software Quadrature Decoding** via TCA8418 key events (not hardware PCNT).

**Rationale**: 
- Encoder used for menu navigation (low-speed, not motor control)
- Fits existing architecture cleanly through event system
- No additional GPIO pins required
- TCA8418 provides hardware debouncing

## Quadrature Decoding

Implements Gray code state machine for direction detection:

```
Clockwise:     00 → 01 → 11 → 10 → 00
Counterclockwise: 00 → 10 → 11 → 01 → 00
```

**State Tracking**:
- Monitors A and B channel state transitions
- Increments/decrements internal position counter
- Publishes `encoder.turned` event on valid transitions

## Events Published

### `encoder.turned` (EventType::EncoderTurned)
```json
{
  "direction": "cw"|"ccw",
  "steps": 1
}
```

### `encoder.pressed` (EventType::EncoderPressed)
```json
{
  "pressed": true|false
}
```

## Usage Example

```cpp
// Subscribe to encoder events
EventBus& bus = eventBus();
bus.subscribe(EventType::EncoderTurned, &handleEncoderTurned, this);
bus.subscribe(EventType::EncoderPressed, &handleEncoderPressed, this);

// Handler
static void handleEncoderTurned(const events::Event& event, void* context) {
  String json = String(event.value.c_str());
  String direction = parseStringField(json, "direction");
  int steps = parseIntField(json, "steps", 1);
  
  if (direction == "cw") {
    // Scroll menu down / increase value
  } else {
    // Scroll menu up / decrease value
  }
}

static void handleEncoderPressed(const events::Event& event, void* context) {
  bool pressed = parseBoolField(event.value, "pressed");
  if (pressed) {
    // Select menu item / confirm action
  }
}
```

## Implementation Details

**File**: `src/RadioHardware.cpp`

**Functions**:
- `handleEncoderEvent(col, pressed)` - Processes TCA8418 key events for encoder
- `publishEncoderTurned(direction, steps)` - Publishes rotation event
- `publishEncoderButton(pressed)` - Publishes button event

**State Management**: `EncoderState` struct tracks:
- `last_a`, `last_b` - Previous channel states (1 bit each)
- `button_pressed` - Button state (1 bit)
- `position` - Cumulative rotation position (int8_t)

## Debugging

Enable debug output in Serial Monitor:
```
[Encoder] Turned CW (1 steps, position=5)
[Encoder] Turned CCW (1 steps, position=4)
[Encoder] Button PRESSED
[Encoder] Button RELEASED
```

## TCA8418 Configuration

Encoder channels configured as part of the 4×10 keypad matrix:
```cpp
static const int ENCODER_ROW = 1;
static const int ENCODER_COL_A = 1;
static const int ENCODER_COL_B = 2;
static const int ENCODER_COL_BUTTON = 3;
```

Hardware debouncing provided by TCA8418 (set filter to maximum: 1023).

## Alternative: Hardware PCNT

For high-speed applications (motor control, high-resolution feedback), use ESP32 PCNT peripheral:
- Wire encoder **directly to ESP32 GPIO pins** (bypass TCA8418)
- Use [ESP32Encoder library](https://github.com/madhephaestus/ESP32Encoder)
- Hardware pulse counting with zero CPU overhead
- Supports up to 8 encoders on standard ESP32

Not necessary for menu navigation.

## ESPHome Integration

See `ESPHome-Integration.md` for exposing encoder events to Home Assistant.

Encoder events bridge to HA for:
- Remote menu navigation
- Volume/brightness control
- Virtual encoder in HA dashboard
