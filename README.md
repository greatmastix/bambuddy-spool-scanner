# Bambuddy Spool Scanner

An ESP32 + RC522 RFID reader that adds Bambu Lab filament spools to
[Bambuddy](https://github.com/maziggy/bambuddy)'s inventory. Hold a spool, or the
unopened box it came in, over the reader: it reads the spool's RFID tag and creates
the spool in Bambuddy with material, colour, weight and temperatures filled in. A
spool that is already in the inventory is recognised and not added twice.

## How it works

Bambu Lab spools carry two MIFARE Classic 1K tags, one on each side of the hub.
Each sector's key is derived from the tag's UID with HKDF-SHA256 (documented by the
[Bambu Research Group](https://github.com/Bambu-Research-Group/RFID-Tag-Guide)), so
the firmware can read the tag without any key table:

1. Wait for a tag, derive its 16 sector keys from the UID.
2. Read sectors 0–4 (blocks 0–16) and decode material, colour, weight, nozzle
   temperatures, the material id (`GFA00`) and the **tray UUID**, the same id the
   AMS reports for that spool.
3. `GET /api/v1/inventory/spools/by-tag?tray_uuid=…&tag_uid=…`. If Bambuddy already
   has the spool, blink twice and stop.
4. Otherwise `POST /api/v1/inventory/spools`, with fields derived the same way
   Bambuddy derives them when an AMS reports the spool, and the colour name looked
   up from Bambuddy's colour catalogue. Because `tray_uuid` is set, Bambuddy links
   the spool to the AMS slot automatically when you load it later.

Both tags of a spool share the tray UUID, and the same spool is ignored for 15 s
after a scan, so holding a box over the reader adds it exactly once.

## Hardware

- ESP32 DevKit (ESP32-WROOM-32)
- RC522 (MFRC522) 13.56 MHz module
- optional: active 3.3 V piezo buzzer

### Wiring

The RC522 runs on **3.3 V only**; connecting it to 5 V destroys it.

| Part | Pin | ESP32 |
|------|-----|-------|
| RC522 | SDA | GPIO 16 |
| RC522 | SCK | GPIO 17 |
| RC522 | MOSI | GPIO 5 |
| RC522 | MISO | GPIO 18 |
| RC522 | IRQ | not connected (or GPIO 19, unused) |
| RC522 | GND | GND |
| RC522 | RST | GPIO 22 |
| RC522 | 3.3V | 3V3 |
| Buzzer (optional) | + | GPIO 25 |
| Buzzer (optional) | − | GND |

The RC522's header lines up 1:1 with the DevKit row `16 17 5 18 19 21 22 23`,
except GND and 3.3V, which go to the ESP32's GND and 3V3 pins. GPIO 19, 21 and 23
are deliberately left unused. Details in [docs/wiring.md](docs/wiring.md).

- The buzzer must be an **active** 3.3 V buzzer (beeps when DC is applied); a
  passive one only clicks. It is enabled by default; GPIO 25 with nothing attached
  is harmless.
- Status LED: the DevKit's on-board LED on GPIO 2. Setup button: the DevKit's BOOT
  button. Neither needs wiring.

Diagram and mounting tips for reading through the box: [docs/wiring.md](docs/wiring.md).

### Will the RC522 read through the box?

Probably, with some care. The tag sits in the spool flange close to the hub, so from
outside the box it is the carton wall (~3–5 mm) plus the flange, roughly 5–10 mm in
total. A typical RC522 reads a credit-card-sized tag at 3–5 cm, but Bambu's tags are
much smaller, which cuts that to roughly 1.5–3 cm. That is enough, but only when the
tag is right over the antenna:

- Put the box on the reader with the spool's hub centred over the antenna, then
  slowly turn or slide it in a small circle. The tags sit at a fixed radius around the
  hub, but you cannot see where.
- The firmware sets the RC522 receiver gain to maximum.
- An opened spool (tag directly on the reader) reads instantly.

If it turns out unreliable through the box, the upgrade worth buying is a **PN5180**
module (noticeably more range; it is what Bambuddy's own
[SpoolBuddy](https://github.com/maziggy/bambuddy/tree/main/spoolbuddy) station uses).
A PN532 is not worth swapping to: its range is about the same as the RC522's.

## Flash from the browser

Open **https://greatmastix.github.io/bambuddy-spool-scanner/** in Chrome or Edge,
plug in the ESP32 and click **Connect**. The page is built from `main` by the
`web flasher` workflow using [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Build and flash

With [PlatformIO](https://platformio.org/):

```sh
pio run -e esp32dev -t upload
pio device monitor
```

Run the host-side tests (key derivation and decoding, checked against real tag dumps):

```sh
pio test -e native
```

## Setup

1. In Bambuddy, create an API key under **Settings → API Keys** with the
   **Manage inventory** permission. (If Bambuddy has authentication turned off, you
   can leave the key empty.)
2. Flash with the [web installer](https://greatmastix.github.io/bambuddy-spool-scanner/).
   When flashing finishes it asks for your WiFi (via
   [Improv Wi-Fi](https://www.improv-wifi.com/), the same way WLED does it).
3. Click **Visit device**. The scanner's settings page opens at
   `http://<scanner-ip>/param`. Fill in:
   - **Bambuddy URL**, e.g. `http://192.168.1.50:8000`
   - **API key**
   - **Storage location** (optional, written to every new spool)
4. Save. The scanner is ready.

The settings page stays available at `http://<scanner-ip>/param`.

Without the web installer: when the scanner has no working WiFi it opens an access
point called **SpoolScanner-Setup**. Join it and the same setup page opens (or browse
to `192.168.4.1`). Holding the **BOOT** button for 3 seconds reopens that access point,
for example to change WiFi.

## Using it

| LED / buzzer | Meaning |
|--------------|---------|
| LED on + short chirp | tag detected, reading: **hold still** until the result |
| one long blink | spool added to Bambuddy |
| two short blinks | spool is already in Bambuddy |
| three quick blinks | tag moved away mid-read: hold it still and try again |
| five fast blinks | error, e.g. Bambuddy unreachable (see the status page) |
| three slow blinks | setup access point open |

If the tag slips out of range during a read, the scanner re-selects it and retries a
few times before giving up, so a slight wobble usually doesn't matter.

The scanner's web page (`http://<scanner-ip>/`, also shown on the settings page)
shows whether Bambuddy is connected and accepts the API key, whether the RC522
responds, and the last scan. The Bambuddy check runs every minute, after saving the
settings, and when you click **check now**.

The serial log shows every scan, for example:

```
Spool: PLA Basic | colour #0A2989FF | 1000 g | 1.75 mm | nozzle 190-230 C
       GFA00 / A00-B9 | tag 138A2939 | tray 509BB15DC6F842D19E9D8C17FA834813 | made 2025-01-16 12:09
Bambuddy: created spool #42
```

## What gets written to Bambuddy

| Bambuddy field | From the tag |
|----------------|--------------|
| `material`, `subtype` | filament type + detailed type (`PLA` + `PLA Basic` → `PLA` / `Basic`) |
| `rgba`, `color_name` | colour, name from Bambuddy's colour catalogue |
| `extra_colors`, `effect_type` | second colour on dual-colour spools, finish from subtype |
| `label_weight` | spool weight (g) |
| `nozzle_temp_min/max` | hotend temperatures |
| `slicer_filament`, `slicer_filament_name` | material id (`GFA00`), detailed type |
| `tag_uid`, `tray_uuid` | tag UID, tray UUID |
| `brand`, `tag_type`, `data_origin` | `Bambu Lab`, `bambulab`, `rfid_scanner` |
| `note` | production date |

Only Bambu Lab tags are supported; other MIFARE cards are ignored.

## Project layout

```
lib/BambuTag/   key derivation (HKDF-SHA256) and tag decoding, no Arduino dependency
src/            firmware: RC522 reader, Bambuddy client, WiFi setup portal, LED feedback
test/           host-side tests against real tag dumps
docs/           wiring
web/            browser flasher page (published to GitHub Pages)
```

## Credits

- Tag format and key derivation: [Bambu Research Group RFID Tag Guide](https://github.com/Bambu-Research-Group/RFID-Tag-Guide)
- Test fixtures: [Bambu Lab RFID Library](https://github.com/queengooborg/Bambu-Lab-RFID-Library)
- [Bambuddy](https://github.com/maziggy/bambuddy)
