# ESPHome Component Implementation

## Goal

Create two reusable ESPHome components:
1. **`tca8418_keypad`** - I2C keyboard matrix controller (high community value, focus first)
2. **`retrotext_display`** - RetroText 3-board LED matrix display platform (after keypad works)

## ESPHome Component Requirements

Per the [ESPHome documentation](https://developers.esphome.io/architecture/components/):

1. **Flat file structure** - All `.h` and `.cpp` files must be at component root (no subdirectories)
2. **Component directory structure**:
   ```
   components/my_component/
   ├── __init__.py          # Python config validation & code generation
   ├── my_component.h       # C++ header
   ├── my_component.cpp     # C++ implementation
   └── [other flat files]   # Additional headers/sources (flat)
   ```
3. **Local path usage** - Development uses relative paths: `source: ../components`
4. **Namespace matching** - Python namespace must match C++ namespace and folder name
5. **I2C Components** - Must inherit from `i2c::I2CDevice` and use ESPHome's I2C abstraction
6. **Self-contained** - Embed necessary driver code; avoid external library dependencies

## Current Project Structure

```
radio-retrofit-asc-tuner/
├── src/
│   ├── hardware/RadioHardware.cpp
│   ├── display/DisplayManager.cpp
│   ├── platform/InputManager.cpp
│   └── main.cpp
├── include/
│   ├── hardware/RadioHardware.h
│   ├── display/DisplayManager.h
│   └── platform/InputManager.h
├── lib/
│   ├── IS31FL373x/
│   └── Adafruit_TCA8418/
├── test/
│   └── [native tests]
└── esphome/
    ├── components/
    │   └── radio_controller/  # Old attempt, will be replaced
    └── devices/
        └── radio.yaml
```

## Target Structure After Refactoring

```
radio-retrofit-asc-tuner/
├── esphome/
│   ├── components/
│   │   ├── tca8418_keypad/           # New component 1
│   │   │   ├── __init__.py
│   │   │   ├── binary_sensor.py
│   │   │   ├── tca8418_keypad.h
│   │   │   ├── tca8418_keypad.cpp
│   │   │   ├── Adafruit_TCA8418.h   # Flattened from lib/
│   │   │   ├── Adafruit_TCA8418.cpp
│   │   │   └── Adafruit_TCA8418_registers.h
│   │   └── retrotext_display/        # New component 2
│   │       ├── __init__.py
│   │       ├── display.py
│   │       ├── retrotext_display.h
│   │       ├── retrotext_display.cpp
│   │       ├── IS31FL373x.h         # Flattened from lib/
│   │       ├── IS31FL373x.cpp
│   │       ├── [display files - all flat]
│   │       └── [font files - all flat]
│   └── devices/
│       └── test_components.yaml      # Test config for new components
├── src/                               # Keep as-is for now
├── include/                           # Keep as-is for now
├── lib/                               # Keep as-is for now
└── test/                              # Keep as-is for now
```

## Implementation Plan - TCA8418 Keypad Component (Phase 1)

**Focus:** Get TCA8418 working end-to-end before starting on display component.

**Architecture Decision:**
- **Hybrid approach**: Core component for hardware + triggers, optional binary_sensor platform
- **Embed Adafruit code**: Adapt Adafruit_TCA8418 library for ESPHome I2C (with attribution)
- **ESPHome patterns**: Use `i2c::I2CDevice`, proper lifecycle methods, triggers for events

### Step 1: Component Skeleton & Wireframe

Create minimal component structure that compiles and loads without functionality.

**Tasks:**
1. Create directory structure
2. Create Python files (`__init__.py`) with minimal schema
3. Create C++ skeleton (empty class with required methods)
4. Create test YAML configuration
5. Verify compilation and component registration

**Files to create:**
- `esphome/components/tca8418_keypad/__init__.py` - Component registration
- `esphome/components/tca8418_keypad/tca8418_keypad.h` - Component class skeleton
- `esphome/components/tca8418_keypad/tca8418_keypad.cpp` - Empty implementations
- `esphome/devices/test_tca8418.yaml` - Test configuration

**Success criteria:** 
- `esphome compile` succeeds
- Component appears in logs during initialization
- No runtime errors on ESP32

### Step 2: I2C Communication & Register Interface

Add register definitions and basic I2C read/write using ESPHome's I2C device.

**Tasks:**
1. Copy register definitions from Adafruit library (with attribution)
2. Implement basic I2C read/write methods using ESPHome API
3. Add device detection in `setup()`
4. Test I2C communication with hardware

**Files to create/modify:**
- `esphome/components/tca8418_keypad/tca8418_registers.h` - Register definitions
- Update `tca8418_keypad.cpp` - Add I2C methods and device detection

**Success criteria:**
- Component detects TCA8418 on I2C bus
- Can read device ID register
- Logs show successful communication

### Step 3: Matrix Configuration & Initialization

Port matrix configuration code from Adafruit library.

**Tasks:**
1. Add matrix configuration (rows/columns) to Python schema
2. Port matrix setup code from Adafruit library
3. Implement proper initialization sequence
4. Add `dump_config()` to show configuration

**Success criteria:**
- Matrix configuration accepted in YAML
- Hardware properly configured for matrix scanning
- Configuration logged correctly

### Step 4: Key Event Reading & Processing

Implement key event FIFO reading and event processing.

**Tasks:**
1. Port key event methods from Adafruit library
2. Implement polling in `loop()` method
3. Add event processing and logging
4. Test key press/release detection

**Success criteria:**
- Key presses detected and logged
- Key releases detected and logged
- Events processed correctly from FIFO

### Step 5: ESPHome Triggers & Automations

Add ESPHome automation triggers for key events.

**Tasks:**
1. Define triggers in Python schema (`on_key_press`, `on_key_release`)
2. Implement trigger firing in C++
3. Add key number/row/column to trigger data
4. Test with YAML automations

**Success criteria:**
- Triggers fire on key events
- YAML automations work correctly
- Key data accessible in lambdas

### Step 6: Optional Binary Sensor Platform

Add optional binary_sensor platform for individual keys.

**Tasks:**
1. Create `binary_sensor.py` platform file
2. Add binary sensor registration
3. Implement key-to-sensor mapping
4. Test with individual key sensors

**Files to create:**
- `esphome/components/tca8418_keypad/binary_sensor.py` - Binary sensor platform

**Success criteria:**
- Individual keys exposed as binary sensors
- Sensors update on key state changes
- Multiple sensors work independently

### Step 7: Hardware Testing & Validation

Test with actual ESP32-wrover hardware.

**Tasks:**
1. Deploy to ESP32-wrover with TCA8418 hardware
2. Test all matrix positions
3. Verify timing and responsiveness
4. Test binary sensors and triggers together
5. Document any issues or quirks

**Success criteria:**
- All hardware features working
- No timing issues or missed events
- Ready for production use

## Phase 2: RetroText Display Component (AFTER TCA8418 COMPLETE)

Implement display component using similar approach:
1. Skeleton & wireframe
2. I2C driver integration (IS31FL373x)
3. DisplayBuffer implementation
4. Font rendering
5. Hardware testing

Details to be refined after TCA8418 is complete.

## Component 1: TCA8418 Keypad - File Structure

### Final Structure
```
esphome/components/tca8418_keypad/
├── __init__.py              # Component registration, config schema, triggers
├── binary_sensor.py         # Optional binary sensor platform
├── tca8418_keypad.h         # Main component class
├── tca8418_keypad.cpp       # Component implementation
└── tca8418_registers.h      # Register definitions (from Adafruit)
```

### Source Attribution

Code adapted from [Adafruit_TCA8418](https://github.com/adafruit/Adafruit_TCA8418) library:
- Register definitions (`tca8418_registers.h`) - direct copy with attribution
- Matrix configuration logic - adapted for ESPHome
- Event FIFO handling - adapted for ESPHome

Changes from Adafruit library:
- Replaced `Adafruit_I2CDevice` with ESPHome `i2c::I2CDevice`
- Replaced Arduino `Wire` with ESPHome I2C bus
- Added ESPHome triggers and automations
- Added binary sensor platform support
- ESPHome-style logging and configuration

## Test Configuration

### Test YAML for TCA8418 (`esphome/devices/test_tca8418.yaml`)

```yaml
external_components:
  - source:
      type: local
      path: ../components

esphome:
  name: keypad-test
  friendly_name: TCA8418 Keypad Test

esp32:
  board: esp-wrover-kit
  framework:
    type: arduino

logger:
  level: DEBUG

# I2C bus (matches our hardware)
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 400kHz

# TCA8418 Keypad Component
tca8418_keypad:
  - id: my_keypad
    address: 0x34
    rows: 5
    columns: 8
    
    # Triggers for automation
    on_key_press:
      - logger.log:
          format: "Key pressed: row=%d col=%d key=%d"
          args: ['row', 'col', 'key']
    
    on_key_release:
      - logger.log:
          format: "Key released: row=%d col=%d key=%d"
          args: ['row', 'col', 'key']

# Optional: Expose specific keys as binary sensors
binary_sensor:
  - platform: tca8418_keypad
    tca8418_keypad_id: my_keypad
    row: 0
    column: 0
    name: "Preset Button 1"
  
  - platform: tca8418_keypad
    tca8418_keypad_id: my_keypad
    row: 0
    column: 1
    name: "Preset Button 2"
```

## Build Process

1. **Compile:** `esphome compile esphome/devices/test_components.yaml`
2. **Check logs** for component loading
3. **Iterate** on build errors (includes, namespaces)
4. **Expand** functionality once basic build works

## Critical Success Factors

1. **Start minimal** - Don't copy all features at once
2. **Flatten includes** - All relative paths become flat
3. **One component at a time** - Get TCA8418 working first
4. **Test often** - Compile after every change
5. **Keep notes** - Document any issues or quirks discovered

## What We're NOT Doing (Phase 1 - TCA8418)

- ❌ GPIO expander functionality (focus on matrix only)
- ❌ Advanced debounce configuration (use defaults)
- ❌ Key lock features
- ❌ Complex key mapping (row/col is sufficient)
- ❌ RetroText display component (Phase 2)
- ❌ Community publishing (validate first)

## Phase 1 Status: ✅ COMPLETE

All 7 steps of TCA8418 keypad component implementation have been completed and hardware-validated.

**Completed Steps:**
- ✅ Step 1: Component skeleton & wireframe
- ✅ Step 2: I2C communication & register interface
- ✅ Step 3: Matrix configuration & initialization
- ✅ Step 4: Key event reading & processing
- ✅ Step 5: ESPHome triggers & automations
- ✅ Step 6: Optional binary sensor platform
- ✅ Step 7: Hardware testing & validation on ESP32-wrover

**Hardware Validation Results (2025-10-05):**
- All 8 preset buttons tested (Row 3)
- Encoder button and rotation tested (Row 2)
- Mode selector tested (Row 2)
- Binary sensors working correctly
- ESPHome triggers firing on all events
- No missed events or timing issues
- Press/release detection 100% accurate

**Component Status:** Production-ready, fully functional

**Next Phase:** RetroText Display Component (Phase 2) - To be planned