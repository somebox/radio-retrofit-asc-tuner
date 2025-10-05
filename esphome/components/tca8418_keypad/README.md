# TCA8418 Keypad Component for ESPHome

An ESPHome external component for the TCA8418 I2C keypad matrix controller, supporting up to 8×10 matrix scanning with event-based key detection.

## Features

- ✅ I2C communication using ESPHome native APIs
- ✅ Configurable matrix size (up to 8 rows × 10 columns)
- ✅ Event-based key detection (press and release)
- ✅ ESPHome automation triggers (`on_key_press`, `on_key_release`)
- ✅ Optional binary sensor platform for individual keys
- ✅ Hardware-validated on ESP32
- ✅ No external library dependencies

## Hardware Requirements

- TCA8418 I2C keypad matrix controller
- I2C bus (SDA/SCL)
- ESP32, ESP8266, or other ESPHome-compatible board

**Default I2C Address:** `0x34`

## Installation

### As External Component

Add to your ESPHome YAML configuration:

```yaml
external_components:
  - source:
      type: local
      path: components  # Adjust path to your components directory
    components: [tca8418_keypad]
```

## Basic Configuration

```yaml
# I2C bus configuration
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz

# TCA8418 Keypad
tca8418_keypad:
  id: my_keypad
  address: 0x34
  rows: 4
  columns: 10
```

## Configuration Options

### Main Component

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `id` | ID | Yes | - | Component ID for reference |
| `address` | int | No | `0x34` | I2C address of TCA8418 |
| `rows` | int | No | `8` | Number of matrix rows (1-8) |
| `columns` | int | No | `10` | Number of matrix columns (1-10) |
| `on_key_press` | Automation | No | - | Trigger on key press events |
| `on_key_release` | Automation | No | - | Trigger on key release events |

### Trigger Variables

Both `on_key_press` and `on_key_release` triggers provide these variables:

- `row` (uint8): Zero-based row number (0-7)
- `col` (uint8): Zero-based column number (0-9)
- `key` (uint8): One-based key code (row × 10 + col + 1)

## Usage Examples

### Example 1: Basic Key Logging

```yaml
tca8418_keypad:
  id: my_keypad
  address: 0x34
  rows: 4
  columns: 10
  
  on_key_press:
    - logger.log:
        format: "Key pressed: row=%d, col=%d, key=%d"
        args: ['row', 'col', 'key']
  
  on_key_release:
    - logger.log:
        format: "Key released: row=%d, col=%d, key=%d"
        args: ['row', 'col', 'key']
```

### Example 2: Home Assistant Service Calls

```yaml
tca8418_keypad:
  id: my_keypad
  rows: 4
  columns: 10
  
  on_key_press:
    - homeassistant.service:
        service: script.handle_keypad_press
        data:
          button_row: !lambda 'return row;'
          button_col: !lambda 'return col;'
          button_key: !lambda 'return key;'
```

### Example 3: Conditional Actions by Key

```yaml
tca8418_keypad:
  id: my_keypad
  rows: 4
  columns: 10
  
  on_key_press:
    - if:
        condition:
          lambda: 'return (row == 3 && col == 3);'
        then:
          - logger.log: "Preset Button 1 pressed!"
    - if:
        condition:
          lambda: 'return (row == 3 && col == 2);'
        then:
          - logger.log: "Preset Button 2 pressed!"
```

### Example 4: Binary Sensors for Individual Keys

Expose specific keys as binary sensors in Home Assistant:

```yaml
tca8418_keypad:
  id: my_keypad
  rows: 4
  columns: 10

binary_sensor:
  # Preset buttons
  - platform: tca8418_keypad
    tca8418_keypad_id: my_keypad
    name: "Preset Button 1"
    row: 3
    column: 3
    device_class: button
  
  - platform: tca8418_keypad
    tca8418_keypad_id: my_keypad
    name: "Preset Button 2"
    row: 3
    column: 2
    device_class: button
  
  # Encoder button
  - platform: tca8418_keypad
    tca8418_keypad_id: my_keypad
    name: "Encoder Button"
    row: 2
    column: 2
    device_class: button
```

### Binary Sensor Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `tca8418_keypad_id` | ID | Yes | - | ID of parent keypad component |
| `row` | int | Yes | - | Row number (0-7) |
| `column` | int | Yes | - | Column number (0-9) |
| `name` | string | Yes | - | Sensor name |
| All standard [Binary Sensor](https://esphome.io/components/binary_sensor/) options | | | | |

## Key Code Calculation

The `key` variable in triggers uses a 1-based encoding:

```
key = (row × 10) + col + 1
```

**Examples:**
- Row 3, Col 3 → key 34
- Row 2, Col 2 → key 23
- Row 0, Col 0 → key 1

## Technical Details

### Event Detection

The component polls the TCA8418's event FIFO during each `loop()` cycle (~7000 times/minute). Events are processed immediately with no polling delay.

### Press/Release Decoding

Event bytes from the TCA8418 use bit 7 to indicate press/release:
- **Bit 7 = 1**: Key press
- **Bit 7 = 0**: Key release

See `TCA8418_EVENT_FORMAT.md` for detailed event format documentation.

### Matrix Configuration

The component automatically configures the TCA8418 registers to:
- Set specified rows and columns as matrix pins
- Enable key event interrupts
- Configure event FIFO
- Flush any pending events on startup

## Troubleshooting

### Device Not Detected

```
[E][tca8418_keypad]: Failed to detect TCA8418 device at address 0x34
```

**Solutions:**
- Verify I2C wiring (SDA, SCL, GND, VCC)
- Check I2C address (use `i2c: scan: true`)
- Ensure 4.7kΩ pull-up resistors on SDA/SCL
- Verify TCA8418 power supply (3.3V)

### No Key Events

**Check:**
- Matrix is properly wired to TCA8418 pins
- Row/column configuration matches your hardware
- Keys are making proper contact
- Check debug logs for event bytes

### Wrong Key Mappings

Verify your row/column configuration matches the physical wiring. The TCA8418 uses:
- **Rows**: Pins ROW0-ROW7
- **Columns**: Pins COL0-COL9

## Attribution

This component is based on the [Adafruit_TCA8418](https://github.com/adafruit/Adafruit_TCA8418) library, adapted for ESPHome with the following changes:
- Replaced `Adafruit_I2CDevice` with ESPHome's `i2c::I2CDevice`
- Added ESPHome automation triggers
- Added binary sensor platform support
- Removed GPIO expander features (focus on matrix scanning only)

Original library: Copyright (c) 2020 Adafruit Industries (BSD License)

## Examples

See `test_tca8418.yaml` in the `esphome/devices/` directory for a complete working example.

## License

This component follows the same BSD license as the original Adafruit library. See LICENSE file for details.

## Status

**Production Ready** - Hardware validated on ESP32-wrover with 4×10 matrix (2025-10-05)

## Support

For issues or questions:
1. Check the `TCA8418_EVENT_FORMAT.md` documentation
2. Review example configurations
3. Enable debug logging: `logger: level: DEBUG`
