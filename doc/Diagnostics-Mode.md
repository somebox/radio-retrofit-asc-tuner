# Diagnostics Mode

Interactive serial console for hardware testing and debugging.

## Concept

**Purpose**: Provide immediate hardware debugging when things fail, without recompiling.

**Design Decisions**:
- Serial input handled in main.cpp (single coordination point)
- Auto-activate on boot failures (immediate debugging access)
- Suppress periodic logs when active (clean console)
- Skip normal operations when active (no interference)

## Commands

### LED Control

```
led <row> <col> <brightness>  - Set specific LED (0-255)
led all <brightness>           - Set all LEDs
led test                       - Cycle through all LEDs
led clear                      - Clear all LEDs
```

**Examples**:
```
led 0 0 255        # Full brightness on SW0, CS0
led 1 3 128        # Half brightness on SW1, CS3
led all 0          # Clear all
led test           # Visual test pattern
```

### Control Monitoring

```
controls                    - Show current button, encoder and switch states
```

**Output**:
```
[Encoder] Turned CW (1 steps)
[Encoder] Button PRESSED
[Switch] Stereo (0)
[Button] Preset 1 (0)
```

### Event System

```
events                     - Show event catalog and start monitoring session
```

### System Information

```
info                       - Show system information, memory and uptime
config                     - Show configuration options
```

**Output**:
```
Radio Retrofit Diagnostics Mode
Firmware: v1.0.0
Uptime: 12m 34s
Free Heap: 245KB / 320KB
Keypad: OK (TCA8418)
Display: OK (3x IS31FL3737)
Preset LEDs: OK (IS31FL3737)
Events: 9 types, 15 active subscribers
```

### Menu, Help & Navigation

Diagnostics mode is enabled by default, and activated by any keypress in Serial Monitor (when no menu active). The menu is displayed as a list of commands. 

```
help or ?                      - Show command list
exit or q                      - Exit diagnostics mode
```

---

## Integration Pattern

### Serial Handling in Main

Main reads serial line-by-line, passes to `diagnostics.processCommand()`.

### Boot Failure Auto-Activation

```cpp
if (!radio_hardware.initialize()) {
  #ifdef ENABLE_DIAGNOSTICS
    diagnostics.activate("Hardware init failed");
    return;  // Stay in diagnostics
  #endif
}
```

### Log Suppression

```cpp
// Wrap periodic logs
#ifdef ENABLE_DIAGNOSTICS
  if (!diagnostics.isActive())
#endif
{
  Serial.printf("FPS: %d\n", fps);
}
```

---
