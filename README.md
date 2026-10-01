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
- optional: active piezo buzzer

Wiring and mounting tips: [docs/wiring.md](docs/wiring.md).

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
2. Power the scanner. On first boot it opens a WiFi access point called
   **SpoolScanner-Setup**. Join it; the setup page opens (or browse to
   `192.168.4.1`).
3. Pick your WiFi and fill in:
   - **Bambuddy URL**, e.g. `http://192.168.1.50:8000`
   - **API key**
   - **Storage location** (optional, written to every new spool)
4. Save. The scanner joins your WiFi and is ready.

To change the settings later, hold the **BOOT** button for 3 seconds while the
scanner is running; the setup portal opens again.

## Using it

| LED / buzzer | Meaning |
|--------------|---------|
| on | reading the tag and talking to Bambuddy |
| one long blink | spool added to Bambuddy |
| two short blinks | spool is already in Bambuddy |
| five fast blinks | error (see the serial log at 115200 baud) |
| three slow blinks | setup portal open |

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
```

## Credits

- Tag format and key derivation: [Bambu Research Group RFID Tag Guide](https://github.com/Bambu-Research-Group/RFID-Tag-Guide)
- Test fixtures: [Bambu Lab RFID Library](https://github.com/queengooborg/Bambu-Lab-RFID-Library)
- [Bambuddy](https://github.com/maziggy/bambuddy)
