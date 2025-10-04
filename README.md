# Radio Retrofit 

## ASC AS-5000 Tuner as Internet Streamer

The AS-5000E tuner from the late 1970s was an early digital tuner which featured programmable presets and a modular design. This project is a retrofit which replaces the internal electronics with an ESP32 and music streaming server, retaining the original controls and design. Check out the [vintage product sheet](https://asc6000.de/downloads/asc-hifi/asc-prospekt_as5000e_5000v_1977.pdf).

![Radio Retrofit](./doc/hero.jpg)

The display was originally a 7-segment frequency display, replaced with [RetroText](https://github.com/PixelTheater/retrotext) to form a custom 18-char 4×6 LED matrix display. The buttons and LEDs are controlled with the [Lights n' Buttons](https://github.com/PixelTheater/lights-and-buttons) PCB, which features the IS31FL3737 LED driver (12×11 usable matrix) and the TCA8418 keypad controller. There's a single big tuner knob which has been re-wired to a pushbutton rotary encoder for changing stations, selecting presets, and navigating menus.

## Overview of Firmware

This firmware interfaces with the controls and defines some basic modes and behaviors.

- Startup: scans and tests the required components, initializes the drivers and starting state, with status and animation displayed.
- Defines modes: select presets with buttons and switches, while manages state, timeouts, visual feedback and what mode the user is in.
- Playlist access: allows user to scroll through more detailed playlist, and provides configuration and navigation options
- Source mode: switch between internet radio, local media files, spotify, etc. This interfaces with modules that manage the actual playback and audio output, and provide status feedback and control.

**Management**: `PresetManager` queries `InputManager` for button states, manages mode selection, LED feedback, and announcements.

## Hardware and Controls

- **Display**: 3× [RetroText](https://github.com/PixelTheater/retrotext) modules, each with 6 characters, for a total of 18 4×6 pixel characters. I2C addresses: 0x50, 0x5A, 0x5F (GND, VCC, SDA).
- **LED & Button Panel**: [Lights-and-Buttons](https://github.com/PixelTheater/lights-and-buttons) module with IS31FL3737 LED driver (12×12 matrix) and TCA8418 keypad controller. I2C addresses: 0x55 (LEDs), 0x34 (keypad).
- **Rotary Encoder**: Pushbutton rotary encoder connected to TCA8418 keypad matrix.
- **Power**: 5V external supply, 3.3V logic via ESP32.


## Project Progress

First version: prototype, combining the existing [RetroText](https://github.com/PixelTheater/retrotext) and [Lights n' Buttons](https://github.com/PixelTheater/lights-and-buttons) projects.
Create ESPHome integration.

## Development

**Build ESP32 firmware:**
```bash
pio run -e esp32doit-devkit-v1  # or esp32-wrover
```

**Run tests:**
```bash
pio test -e native  # 30 tests covering input, events, parsing
```

**ESPHome:**
```bash
cd esphome && esphome compile devices/radio.yaml
```

## Project Structure

```
/include
  /hardware     # RadioHardware, PresetManager, HardwareConfig
  /display      # DisplayManager, SignTextController, Fonts
  /features     # AnnouncementModule, ClockDisplay, DiagnosticsMode
  /platform     # InputManager, Events, Time, HomeAssistantBridge
/src            # Implementation files (mirror /include structure)
/test           # Native unit tests
/doc            # Technical documentation
/esphome        # Home Assistant integration
```




