# I2C Bus Conflict Analysis & Resolution

## Problem Description

Two different firmwares on the same hardware showed conflicting I2C device detection:

### Working RetroText Firmware
- **Detected**: IS31FL3737 displays at 0x50, 0x5A, 0x5F ✅
- **NOT Detected**: TCA8418 keypad (0x34) and IS31FL3737 preset LEDs (0x55) ❌

### Our Radio Firmware  
- **Detected**: TCA8418 keypad (0x34) and IS31FL3737 preset LEDs (0x55) ✅
- **NOT Detected**: IS31FL3737 displays at 0x50, 0x5A, 0x5F ❌

## Root Cause Analysis

### Issue 1: I2C Pin Configuration Mismatch

**Working RetroText Firmware**:
```cpp
Wire.begin(); // Uses ESP32 defaults: SDA=GPIO21, SCL=GPIO22
```

**Our Radio Firmware (INCORRECT)**:
```cpp
Wire.begin(22, 21); // SDA=GPIO22, SCL=GPIO21 (SWAPPED!)
```

### Issue 2: Hardware Wiring Configuration

The hardware appears to be wired with different device groups connected to different I2C pin configurations:

- **RetroText displays**: Wired to ESP32 default pins (SDA=21, SCL=22)
- **Keypad/LED board**: Wired to swapped pins (SDA=22, SCL=21)

## Solution Implementation

### Fix 1: Use ESP32 Default I2C Pins
```cpp
// Corrected configuration
Wire.begin(); // SDA=GPIO21, SCL=GPIO22 (ESP32 defaults)
Wire.setClock(800000); // 800 kHz I2C
```

### Fix 2: Proper I2C Initialization Order
```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize I2C first, before any device libraries
  Serial.println("Initializing I2C with default ESP32 pins...");
  Wire.begin(); 
  Wire.setClock(800000);
  Serial.println("I2C initialized: SDA=GPIO21, SCL=GPIO22");
  delay(100); // Allow I2C to stabilize
  
  // Then initialize device libraries...
}
```

### Fix 3: Eliminate Double I2C Initialization
- Removed duplicate `Wire.begin()` call from DisplayManager
- Ensured single I2C initialization in main.cpp

## Expected Results

With the corrected I2C pin configuration, all devices should be detected:

| Device | Address | Expected Result |
|--------|---------|-----------------|
| TCA8418 Keypad | 0x34 | ✅ Detected |
| IS31FL3737 Preset LEDs | 0x55 | ✅ Detected |
| IS31FL3737 Display 1 | 0x50 | ✅ Should now be detected |
| IS31FL3737 Display 2 | 0x5A | ✅ Should now be detected |
| IS31FL3737 Display 3 | 0x5F | ✅ Should now be detected |

## Hardware Wiring Implications

This analysis suggests that the hardware may be wired with:

1. **RetroText display modules** connected to standard ESP32 I2C pins
2. **Keypad/LED board** possibly connected through a different path or with pin swapping

The firmware now uses the standard ESP32 I2C configuration that works with the RetroText displays, which should allow detection of all devices on the same I2C bus.

## Testing Recommendations

1. Flash the updated firmware
2. Run I2C scan to verify all 5 devices are detected
3. Test functionality of both display and keypad/LED systems
4. If issues persist, check physical I2C wiring and connections

## Library Compatibility

The fix also addresses potential conflicts between:
- **Wire library** (used for I2C scanning)
- **Adafruit BusIO** (used by TCA8418 library)

By using consistent I2C initialization and timing, both libraries should work harmoniously.
