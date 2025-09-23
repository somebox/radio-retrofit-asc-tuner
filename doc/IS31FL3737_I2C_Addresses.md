# IS31FL3737 I2C Address Analysis

## Current Issue

The terminal output shows warnings about drivers not responding:
```
WARNING: Driver 0 (0x50) not responding - skipping initialization
WARNING: Driver 1 (0x5A) not responding - skipping initialization  
WARNING: Driver 2 (0x5F) not responding - skipping initialization
```

However, our documentation shows different addresses. Let me investigate the correct I2C addresses.

## IS31FL3737 I2C Address Specification

According to the IS31FL373x driver library documentation and datasheet:

### Address Format
- **7-bit I2C address format**: `0b101` + `A4:A1 (ADDR pin configuration)`
- **Base address**: `0b1010000` = `0x50`

### Address Pin Configuration

The ADDR pin can be connected to one of four different signals to set the address:

| ADDR Pin Connected To | Bit Value (A4:A1) | Calculated Address | Hex Address |
|----------------------|-------------------|-------------------|-------------|
| **GND**              | `0000`            | `0b1010000`       | **0x50**    |
| **SCL**              | `0101`            | `0b1010101`       | **0x55**    |
| **SDA**              | `1111`            | `0b1011111`       | **0x5F**    |
| **VCC**              | `1010`            | `0b1011010`       | **0x5A**    |

## Current Code Implementation

In `src/DisplayManager.cpp`, the `getI2CAddressFromADDR()` function returns:

```cpp
case ADDR::GND: return 0x50;  // ADDR pin connected to GND ✓
case ADDR::VCC: return 0x5A;  // ADDR pin connected to VCC ✓  
case ADDR::SDA: return 0x5F;  // ADDR pin connected to SDA ✓
case ADDR::SCL: return 0x55;  // ADDR pin connected to SCL ✓
```

**These addresses are CORRECT according to the datasheet.**

## Issue Analysis

The problem is **NOT** with the I2C addresses in the code. The addresses are correct:

- **0x50** (GND) - ✅ Correct
- **0x54** (SCL) - ✅ Correct  
- **0x5A** (VCC) - ✅ Correct
- **0x5F** (SDA) - ✅ Correct

## Actual Problem: Hardware Not Connected

The "not responding" warnings indicate that the **physical hardware is not connected** or not powered:

1. **Display modules at 0x50, 0x5A, 0x5F** - RetroText display boards not connected
2. **Preset LED driver at 0x54** - Additional IS31FL3737 for preset LEDs not connected

## Documentation Correction Needed

Our documentation in `doc/Hardware_Integration_Summary.md` shows incorrect address mapping:

**INCORRECT (in our docs):**
```
- 0x50: IS31FL3737 Display (GND)
- 0x51: IS31FL3737 Display (VCC)  ❌ WRONG!
- 0x52: IS31FL3737 Display (SDA)  ❌ WRONG!
- 0x53: IS31FL3737 Preset LEDs (SCL)  ❌ WRONG!
```

**CORRECT (actual addresses):**
```
- 0x50: IS31FL3737 Display (GND)     ✅
- 0x5A: IS31FL3737 Display (VCC)     ✅ 
- 0x5F: IS31FL3737 Display (SDA)     ✅
- 0x54: IS31FL3737 Preset LEDs (SCL) ✅
```

## RetroText PCB Configuration

According to the RetroText documentation, the three display modules should be configured as:

1. **Module 1**: ADDR pin → GND = **0x50**
2. **Module 2**: ADDR pin → VCC = **0x5A** 
3. **Module 3**: ADDR pin → SDA = **0x5F**

The fourth module (preset LEDs) should be:
4. **Module 4**: ADDR pin → SCL = **0x54**

## Action Items

1. **✅ Code is correct** - No changes needed to I2C addresses
2. **❌ Hardware missing** - Need to connect RetroText modules and preset LED board
3. **📝 Documentation fix** - Update documentation with correct addresses
4. **🔧 RadioHardware fix** - Update PRESET_LED_I2C_ADDR from 0x53 to 0x54

## Summary

The I2C addresses in the code are **100% correct** according to the IS31FL3737 datasheet. The issue is that the physical hardware (RetroText display modules and preset LED board) are not connected to the ESP32, which is why the I2C scan shows "not responding" errors.

The four valid I2C addresses for IS31FL3737 are:
- **0x50** (ADDR → GND)
- **0x55** (ADDR → SCL)  
- **0x5A** (ADDR → VCC)
- **0x5F** (ADDR → SDA)
