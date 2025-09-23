# AS-5000E Panel Controls

## Main display

A dot matrix display using [RetroText](https://github.com/PixelTheater/retrotext) to display 18 characters, each is 4x6. The dots are individually addressable with PWM, and the display is driven by the IS31FL3737 LED driver.

## Controls

There's a main knob connected to a pushbutton rotary encoder. This will be used to change the station or preset, or navigate menus and playlists.

Along the bottom are 7 preset buttons, and 1 button labeled "memory". Each of these buttons have an LED above them.

There's a "muting" knob, connected to a potentiometer. This will be used to adjust the volume.

A 4-way selector switch has the options "stereo", "stereo-far", "Q" and "mono".

There's two VU meters, one for "tuning" and the other for "signal".

Finally, there's two headphone jacks paired with a single volume control, and a cassette interface DIN socket. 

## Wiring

The buttons are connected to the TCA8418 keypad controller, and the LEDs are connected to the IS31FL3737 LED driver (address: SCL).

| Control                | Device/Interface         | Connection                | Function                                      |
|------------------------|-------------------------|---------------------------|-----------------------------------------------|
| Main Display           | IS31FL3737 (0x50/0x51/0x52) | I2C bus                   | 18x 4x6 dot-matrix character display          |
| Rotary Encoder (knob)  | TCA8418 (0x34)          | Row/Col (TBD)             | Station/preset/menu navigation (with push)    |
| Preset 1 Button        | TCA8418 (0x34)          | Row 0, Col 0              | Select preset 1                               |
| Preset 1 LED           | IS31FL3737 (SCL)        | SW0, CS0                  | Indicates preset 1 status                     |
| Preset 2 Button        | TCA8418 (0x34)          | Row 0, Col 1              | Select preset 2                               |
| Preset 2 LED           | IS31FL3737 (SCL)        | SW0, CS1                  | Indicates preset 2 status                     |
| Preset 3 Button        | TCA8418 (0x34)          | Row 0, Col 2              | Select preset 3                               |
| Preset 3 LED           | IS31FL3737 (SCL)        | SW0, CS2                  | Indicates preset 3 status                     |
| Preset 4 Button        | TCA8418 (0x34)          | Row 0, Col 3              | Select preset 4                               |
| Preset 4 LED           | IS31FL3737 (SCL)        | SW0, CS3                  | Indicates preset 4 status                     |
| Preset 5 Button        | TCA8418 (0x34)          | Row 0, Col 5              | Select preset 5                               |
| Preset 5 LED           | IS31FL3737 (SCL)        | SW0, CS5                  | Indicates preset 5 status                     |
| Preset 6 Button        | TCA8418 (0x34)          | Row 0, Col 6              | Select preset 6                               |
| Preset 6 LED           | IS31FL3737 (SCL)        | SW0, CS6                  | Indicates preset 6 status                     |
| Preset 7 Button        | TCA8418 (0x34)          | Row 0, Col 7              | Select preset 7                               |
| Preset 7 LED           | IS31FL3737 (SCL)        | SW0, CS7                  | Indicates preset 7 status                     |
| Memory Button          | TCA8418 (0x34)          | Row 0, Col 8              | Store/recall preset                           |
| Memory LED             | IS31FL3737 (SCL)        | SW0, CS8                  | Indicates memory mode                         |
| Muting Knob            | ESP32 ADC               | GPIO (TBD)                | Volume adjustment                             |
| Selector Switch        | ESP32 GPIO              | GPIOs (TBD)               | Selects stereo/mono/Q mode                    |
| Tuning VU Meter        | ESP32 DAC/PWM           | GPIO (TBD)                | Indicates tuning accuracy                     |
| Signal VU Meter        | ESP32 DAC/PWM           | GPIO (TBD)                | Indicates signal strength                     |
| Headphone Jacks (x2)   | Audio Jack              | Audio Output              | Headphone audio output                        |
| Headphone Volume       | ESP32 ADC               | GPIO (TBD)                | Headphone volume control                      |
| Cassette DIN Socket    | DIN Connector           | none       | Cassette interface                            |

