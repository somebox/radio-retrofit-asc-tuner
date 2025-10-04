# ESPHome Component Implementation

## Goal

Create two reusable ESPHome components by extracting and adapting existing code:
1. **`tca8418_keypad`** - I2C keyboard matrix controller (high community value)
2. **`retrotext_display`** - RetroText 3-board LED matrix display platform

## ESPHome Component Requirements

Per the [ESPHome external components documentation](https://esphome.io/components/external_components/):

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

## Implementation Plan

### Phase 1: Proof of Concept (THIS PHASE)

**Goal:** Prove the component structure works with minimal code.

**Step 1: Create minimal `tca8418_keypad` component**
1. Create directory: `esphome/components/tca8418_keypad/`
2. Copy library files from `lib/Adafruit_TCA8418/` (flatten)
3. Create minimal wrapper (`tca8418_keypad.h/cpp`)
4. Create Python registration (`__init__.py`, `binary_sensor.py`)
5. Create test YAML: `esphome/devices/test_components.yaml`
6. Compile: `esphome compile esphome/devices/test_components.yaml`

**Success criteria:** Build completes without errors, component loads.

**Step 2: Create minimal `retrotext_display` component**
1. Create directory: `esphome/components/retrotext_display/`
2. Copy IS31FL373x library from `lib/` (flatten)
3. Copy minimal display files from `src/display/` and `include/display/` (flatten)
4. Create Python registration (`__init__.py`, `display.py`)
5. Update test YAML to include display
6. Compile: `esphome compile esphome/devices/test_components.yaml`

**Success criteria:** Both components build and load successfully.

### Phase 2: Full Implementation (AFTER PROOF OF CONCEPT)

After Phase 1 proves the structure works:
1. Expand `tca8418_keypad` with full keypad matrix support
2. Expand `retrotext_display` with all display features
3. Create examples showing usage
4. Document component APIs
5. Consider rewriting demo app to use new structure

### Phase 3: Testing Strategy (AFTER FULL IMPLEMENTATION)

Reuse existing native tests where possible:
- `test/test_native_input_controls/` - Can test input logic
- Create ESPHome-specific integration tests
- Hardware validation on actual device

## File Mapping

### Component 1: `tca8418_keypad`

**Source files to copy:**
- `lib/Adafruit_TCA8418/Adafruit_TCA8418.h` → `esphome/components/tca8418_keypad/Adafruit_TCA8418.h`
- `lib/Adafruit_TCA8418/Adafruit_TCA8418.cpp` → `esphome/components/tca8418_keypad/Adafruit_TCA8418.cpp`
- `lib/Adafruit_TCA8418/Adafruit_TCA8418_registers.h` → `esphome/components/tca8418_keypad/Adafruit_TCA8418_registers.h`

**New files to create:**
- `__init__.py` - Empty or minimal component registration
- `binary_sensor.py` - Platform file for keypad keys
- `tca8418_keypad.h` - Wrapper around Adafruit library
- `tca8418_keypad.cpp` - ESPHome component implementation

### Component 2: `retrotext_display`

**Source files to copy (flatten all):**
- `lib/IS31FL373x/*.h` → `esphome/components/retrotext_display/`
- `lib/IS31FL373x/*.cpp` → `esphome/components/retrotext_display/`
- `include/display/DisplayManager.h` → `esphome/components/retrotext_display/DisplayManager.h`
- `src/display/DisplayManager.cpp` → `esphome/components/retrotext_display/DisplayManager.cpp`
- `include/display/FontManager.h` → `esphome/components/retrotext_display/FontManager.h`
- `src/display/FontManager.cpp` → `esphome/components/retrotext_display/FontManager.cpp`
- `include/display/fonts/*.h` → `esphome/components/retrotext_display/`
- `src/display/fonts/*.cpp` → `esphome/components/retrotext_display/`

**New files to create:**
- `__init__.py` - Empty or minimal
- `display.py` - Platform file
- `retrotext_display.h` - ESPHome display platform wrapper
- `retrotext_display.cpp` - Implementation

**Include updates needed:**
All `#include "display/..."` and `#include "fonts/..."` must become flat includes:
- `#include "DisplayManager.h"`
- `#include "FontManager.h"`
- etc.

## Test Configuration

### Initial Test YAML (`esphome/devices/test_components.yaml`)

```yaml
external_components:
  - source:
      type: local
      path: ../components

esphome:
  name: component-test
  friendly_name: Component Test

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
  level: DEBUG

i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 400kHz

# Test TCA8418 keypad
binary_sensor:
  - platform: tca8418_keypad
    id: keypad
    address: 0x34
    # Minimal config for testing

# Test RetroText display
display:
  - platform: retrotext_display
    id: retrotext
    # Minimal config for testing
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

## What We're NOT Doing (Yet)

- ❌ Rewriting demo app
- ❌ Full feature parity
- ❌ Hardware integration component (`as5000e_radio`)
- ❌ Music Assistant integration
- ❌ Home Assistant automations
- ❌ Community publishing

These come AFTER we prove the component structure works.

## Next Action

Start with `tca8418_keypad` component skeleton:
1. Create component directory
2. Copy Adafruit library (3 files)
3. Create minimal wrapper
4. Create Python files
5. Attempt first build