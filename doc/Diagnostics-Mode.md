# Diagnostics Mode

Interactive serial console for hardware testing and debugging.

- Actived by pressing any key in Serial Monitor
- Serial input handled in main.cpp (single coordination point)
- Auto-activates on boot failures (immediate debugging access)
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

**Hardware**: Lights-and-Buttons IS31FL3737 LED driver module
- Board labels: SW1-SW12, CS1-CS12
- Software uses 0-based indexing: row 0-11, col 0-11
- Total: 132 addressable LEDs (12×12 matrix)
- Note: CS12 pin is not functional on the IS31FL3737

**Examples**:

```
led 0 0 255        # Full brightness on SW1, CS1 (board labels)
led 2 10 128       # Half brightness on SW3, CS11 (board labels)
led all 200        # Set all 132 LEDs to brightness 200
led test           # Visual test pattern (all positions)
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

Press any key to stop monitoring.

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

**Command Line Editing**:
- **Left/Right arrow keys** - Move cursor within the current line for editing
- **Home/End keys** - Jump to start/end of line
- **Backspace** - Delete character before cursor (works at any position)
- Character insertion works at any cursor position

**Command History Navigation**:
- **Up/Down arrow keys** - Navigate through previous commands (up to 50 stored)
- **ESC** - Clear current line and reset to end of history
- History persists during the diagnostics session
- Duplicate consecutive commands are not added to history

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
