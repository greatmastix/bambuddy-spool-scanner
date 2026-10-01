# Wiring

ESP32 DevKit (ESP32-WROOM-32, 30 or 38 pin) to an RC522 module over SPI.

| RC522 pin | ESP32 pin | Notes |
|-----------|-----------|-------|
| 3.3V      | 3V3       | **3.3 V only.** 5 V destroys the RC522. |
| GND       | GND       | |
| SDA (SS)  | GPIO 5    | chip select |
| SCK       | GPIO 18   | |
| MOSI      | GPIO 23   | |
| MISO      | GPIO 19   | |
| RST       | GPIO 22   | |
| IRQ       | not connected | |

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
 │      3V3 ├───────────────┤ 3.3V   │
 │      GND ├───────────────┤ GND    │
 │   GPIO 5 ├───────────────┤ SDA    │
 │  GPIO 18 ├───────────────┤ SCK    │
 │  GPIO 23 ├───────────────┤ MOSI   │
 │  GPIO 19 ├───────────────┤ MISO   │
 │  GPIO 22 ├───────────────┤ RST    │
 │          │               │ IRQ  x │
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
