// Bambu Lab filament spool RFID tags: key derivation and decoding.
//
// The tags are MIFARE Classic 1K. Every sector's key A is derived from the
// 4-byte tag UID with HKDF-SHA256, and the spool data lives in plain blocks
// 1..16. Layout reference:
//   https://github.com/Bambu-Research-Group/RFID-Tag-Guide/blob/main/BambuLabRfid.md
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

namespace bambu {

constexpr size_t SECTOR_COUNT = 16;
constexpr size_t KEY_LEN = 6;
constexpr size_t BLOCK_LEN = 16;
// Blocks 0..16 carry everything we decode (sectors 0..4).
constexpr size_t DATA_BLOCK_COUNT = 17;
constexpr size_t SECTORS_TO_READ = 5;

// Derives the 16 sector keys (key A) for a tag with the given UID.
void deriveSectorKeys(const uint8_t* uid, size_t uidLen, uint8_t keys[SECTOR_COUNT][KEY_LEN]);

struct TagData {
  uint8_t uid[4];
  char variantId[9];     // block 1[0..7],  e.g. "A00-B9" (AMS tray_id_name)
  char materialId[9];    // block 1[8..15], e.g. "GFA00"  (AMS tray_info_idx)
  char filamentType[17]; // block 2,        e.g. "PLA"    (AMS tray_type)
  char detailedType[17]; // block 4,        e.g. "PLA Basic" (AMS tray_sub_brands)
  uint8_t rgba[4];       // block 5[0..3]
  uint16_t weightGrams;  // block 5[4..5]
  float diameterMm;      // block 5[8..11]
  uint16_t dryingTempC;  // block 6[0..1]
  uint16_t dryingTimeH;  // block 6[2..3]
  uint16_t bedTempC;     // block 6[6..7]
  uint16_t nozzleMaxC;   // block 6[8..9]
  uint16_t nozzleMinC;   // block 6[10..11]
  uint8_t trayUuid[16];  // block 9 (AMS tray_uuid, shared by both tags of a spool)
  char productionDate[17];  // block 12, "YYYY_MM_DD_hh_mm"
  uint16_t colorCount;      // block 16[2..3]
  uint8_t secondaryRgba[4]; // block 16[4..7], stored ABGR on the tag
};

// Decodes blocks 0..16 (DATA_BLOCK_COUNT * 16 bytes, block-major).
// Returns false if the data does not look like a Bambu Lab tag.
bool decodeTag(const uint8_t blocks[DATA_BLOCK_COUNT][BLOCK_LEN], TagData& out);

// Upper-case hex string of a byte buffer.
std::string toHex(const uint8_t* data, size_t len);

// The fields Bambuddy's POST /inventory/spools wants, derived the same way
// Bambuddy derives them from an AMS tray report (spool_tag_matcher.py).
struct SpoolInfo {
  std::string material;           // "PLA"
  std::string subtype;            // "Basic", "Silk", "Gradient", ... (may be empty)
  std::string rgba;               // "0A2989FF"
  std::string extraColors;        // "00918bff" for dual-colour spools, else empty
  std::string effectType;         // Bambuddy effect vocabulary, else empty
  std::string slicerFilament;     // "GFA00"
  std::string slicerFilamentName; // "PLA Basic"
  std::string detailedType;       // "PLA Basic", used for colour-name lookup
  int labelWeight;                // grams
  int nozzleTempMin;
  int nozzleTempMax;
  std::string tagUid;             // "138A2939"
  std::string trayUuid;           // 32 hex chars
  std::string productionDate;     // "2025-01-16 12:09"
};

SpoolInfo toSpoolInfo(const TagData& tag);

}  // namespace bambu
