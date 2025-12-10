# IR Receiver & Pre-Amp Control

An IR receiver and transmitter control a nearby pre-amplifier and line selector. The pre-amp can be turned on, switched to the right input, and volume adjusted via Home Assistant buttons.

## GPIO Assignments

| Function | GPIO | Notes |
|----------|------|-------|
| IR Receiver | GPIO25 | Inverted input, NEC protocol |
| IR Transmitter | GPIO26 | 50% carrier duty cycle |

## Pre-Amp NEC Commands

All commands use address `0x87EE`:

| Button | Command | Function |
|--------|---------|----------|
| Power | 0xAA04 | Toggle on/off |
| Vol Up | 0xAA0B | Increase volume |
| Vol Down | 0xAA0D | Decrease volume |
| Input Left | 0xAA08 | Previous input |
| Input Right | 0xAA07 | Next input |
| Mute | 0xAA5D | Toggle mute |
| Menu | 0xAA02 | Open menu |

## Configuration

The IR subsystem is configured in `esphome/devices/radio.yaml`. Buttons are exposed to Home Assistant for automation.

To debug incoming IR signals, check the ESPHome logs - received NEC codes are dumped automatically.