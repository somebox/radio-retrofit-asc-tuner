# RetroText PCB Layout

## Physical Dimensions

Each RetroText module displays 6 characters (4×6 pixels each):
- Character size: 15mm high × 8mm wide
- Spacing: 1.5mm between characters
- Driver: IS31FL3737 LED matrix (12×12)

## Character to Pixel Mapping

| Character | Pixel Range | Rows | Cols | Position |
|-----------|-------------|------|------|----------|
| 0 | SW1,CS1 → SW6,CS4 | SW1-6 | CS1-4 | Left |
| 1 | SW1,CS5 → SW6,CS8 | SW1-6 | CS5-8 | |
| 2 | SW1,CS9 → SW6,CS12 | SW1-6 | CS9-12 | |
| 3 | SW7,CS1 → SW12,CS4 | SW7-12 | CS1-4 | |
| 4 | SW7,CS5 → SW12,CS8 | SW7-12 | CS5-8 | |
| 5 | SW7,CS9 → SW12,CS12 | SW7-12 | CS9-12 | Right |

## Usage Notes

- 3 modules arranged horizontally = 18 characters total display
- Each module uses one IS31FL3737 driver (addresses: 0x50, 0x5A, 0x5F)
- Coordinate system: X=column (CS), Y=row (SW)
- Hardware details: [RetroText GitHub](https://github.com/PixelTheater/retrotext)

