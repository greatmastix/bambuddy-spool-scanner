# Wiring

ESP32 DevKit (ESP32-WROOM-32, 30 or 38 pin) to an RC522 module over SPI.

| RC522 pin | ESP32 pin | Notes |
|-----------|-----------|-------|
| SDA (SS)  | GPIO 16   | chip select |
| SCK       | GPIO 17   | |
| MOSI      | GPIO 5    | |
| MISO      | GPIO 18   | |
| IRQ       | (GPIO 19) | not used |
| GND       | GND       | (sits under GPIO 21) |
| RST       | GPIO 22   | |
| 3.3V      | 3V3       | **3.3 V only**, 5 V destroys the RC522. (sits under GPIO 23) |

The pins are chosen so the RC522's 8-pin header lines up 1:1 with the DevKit
row `16 17 5 18 19 21 22 23` (on a 30-pin DevKit that is the row with
`15 2 4 16 17 5 18 19 21 RX TX 22 23`):

```
 RC522:   SDA  SCK  MOSI MISO IRQ  GND  RST  3.3V
 ESP32:   16   17   5    18   19   21   22   23
          ✓    ✓    ✓    ✓    –    ✗    ✓    ✗
```

- ✓ connect straight across.
- – IRQ is not used; leaving it on GPIO 19 is harmless.
- ✗ **GND and 3.3V must go to the ESP32's GND and 3V3 pins**, not to GPIO 21/23.
  With individual jumper wires, just run those two elsewhere. If you solder the
  RC522 straight onto the row, GPIO 21 and 23 end up tied to GND and 3.3 V. That
  is safe only because the firmware never uses them, so don't reuse those two
  pins for anything else.

Optional:

| Part | ESP32 pin | Notes |
|------|-----------|-------|
| Status LED | GPIO 2 | on-board blue LED on most DevKits, nothing to wire |
| Active 3.3 V piezo buzzer (+) | GPIO 25 | enabled by default; buzzer (-) to GND. Must be an *active* buzzer (beeps on DC) |
| Setup button | GPIO 0 | the DevKit's BOOT button, nothing to wire |

All pins can be changed in `src/pins.h` or via `build_flags` in `platformio.ini`.

```
 ESP32 DevKit                RC522
 ┌──────────┐               ┌────────┐
 │  GPIO 16 ├───────────────┤ SDA    │
 │  GPIO 17 ├───────────────┤ SCK    │
 │   GPIO 5 ├───────────────┤ MOSI   │
 │  GPIO 18 ├───────────────┤ MISO   │
 │          │               │ IRQ  x │
 │      GND ├───────────────┤ GND    │
 │  GPIO 22 ├───────────────┤ RST    │
 │      3V3 ├───────────────┤ 3.3V   │
 │          │               └────────┘
 │          │               active buzzer (optional)
 │  GPIO 25 ├───────────────(+)
 │      GND ├───────────────(-)
 └──────────┘
```

## Mounting for reading through the box

- Mount the RC522 with its antenna side (the side with the printed coil, not the
  component side) facing up, directly under a thin lid: 1–2 mm of PLA or acrylic.
  Every millimetre between antenna and tag counts.
- Keep metal (screws, a metal shelf, the ESP32 board itself) at least 2–3 cm away
  from the antenna; metal detunes it and kills the range.
- Keep the wires to the RC522 short (under ~20 cm) for reliable SPI.
