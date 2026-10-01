#include "bambu_tag.h"

#include <ctype.h>
#include <string.h>

#include "sha256.h"

namespace bambu {

namespace {

const uint8_t MASTER_KEY[16] = {0x9a, 0x75, 0x9c, 0xf2, 0xc4, 0xf7, 0xca, 0xff,
                                0x22, 0x2c, 0xb9, 0x76, 0x9b, 0x41, 0xbc, 0x96};
const uint8_t KDF_INFO[] = {'R', 'F', 'I', 'D', '-', 'A', '\0'};

uint16_t u16le(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

float f32le(const uint8_t* p) {
  uint32_t v = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
  float f;
  memcpy(&f, &v, sizeof(f));
  return f;
}

// Copies a NUL-padded string field, stopping at the first non-printable byte.
void copyString(char* dst, size_t dstSize, const uint8_t* src, size_t srcLen) {
  size_t n = 0;
  while (n < srcLen && n + 1 < dstSize && src[n] >= 0x20 && src[n] < 0x7f) {
    dst[n] = (char)src[n];
    n++;
  }
  dst[n] = '\0';
}

std::string lowerHex(const uint8_t* data, size_t len) {
  std::string s = toHex(data, len);
  for (char& c : s) c = (char)tolower((unsigned char)c);
  return s;
}

// Mirrors Bambuddy's ALLOWED_EFFECT_TYPES / normalize_effect_type.
std::string effectFromSubtype(const std::string& subtype) {
  static const char* const allowed[] = {"sparkle", "wood",  "marble",      "glow",     "matte",
                                        "silk",    "galaxy", "rainbow",    "metal",    "translucent",
                                        "gradient", "dual-color", "tri-color", "multicolor"};
  std::string candidates[2] = {subtype, subtype};
  while (!candidates[1].empty() && candidates[1].back() == '+') candidates[1].pop_back();
  for (std::string c : candidates) {
    for (char& ch : c) {
      ch = (char)tolower((unsigned char)ch);
      if (ch == ' ' || ch == '_') ch = '-';
    }
    for (const char* a : allowed) {
      if (c == a) return c;
    }
  }
  return "";
}

}  // namespace

void deriveSectorKeys(const uint8_t* uid, size_t uidLen, uint8_t keys[SECTOR_COUNT][KEY_LEN]) {
  // Same as pycryptodome HKDF(uid, 6, MASTER_KEY, SHA256, 16, context=b"RFID-A\0"):
  // ikm = UID, salt = master key, 96 bytes of output split into 16 six-byte keys.
  uint8_t okm[SECTOR_COUNT * KEY_LEN];
  hkdfSha256(uid, uidLen, MASTER_KEY, sizeof(MASTER_KEY), KDF_INFO, sizeof(KDF_INFO), okm,
             sizeof(okm));
  for (size_t i = 0; i < SECTOR_COUNT; i++) memcpy(keys[i], okm + i * KEY_LEN, KEY_LEN);
}

std::string toHex(const uint8_t* data, size_t len) {
  static const char digits[] = "0123456789ABCDEF";
  std::string s;
  s.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    s.push_back(digits[data[i] >> 4]);
    s.push_back(digits[data[i] & 0x0f]);
  }
  return s;
}

bool decodeTag(const uint8_t blocks[DATA_BLOCK_COUNT][BLOCK_LEN], TagData& out) {
  memset(&out, 0, sizeof(out));
  memcpy(out.uid, blocks[0], 4);

  copyString(out.variantId, sizeof(out.variantId), blocks[1], 8);
  copyString(out.materialId, sizeof(out.materialId), blocks[1] + 8, 8);
  copyString(out.filamentType, sizeof(out.filamentType), blocks[2], 16);
  copyString(out.detailedType, sizeof(out.detailedType), blocks[4], 16);

  memcpy(out.rgba, blocks[5], 4);
  out.weightGrams = u16le(blocks[5] + 4);
  out.diameterMm = f32le(blocks[5] + 8);

  out.dryingTempC = u16le(blocks[6] + 0);
  out.dryingTimeH = u16le(blocks[6] + 2);
  out.bedTempC = u16le(blocks[6] + 6);
  out.nozzleMaxC = u16le(blocks[6] + 8);
  out.nozzleMinC = u16le(blocks[6] + 10);

  memcpy(out.trayUuid, blocks[9], 16);
  copyString(out.productionDate, sizeof(out.productionDate), blocks[12], 16);

  if (u16le(blocks[16]) == 0x0002) {
    out.colorCount = u16le(blocks[16] + 2);
    const uint8_t* abgr = blocks[16] + 4;
    out.secondaryRgba[0] = abgr[3];
    out.secondaryRgba[1] = abgr[2];
    out.secondaryRgba[2] = abgr[1];
    out.secondaryRgba[3] = abgr[0];
  }

  bool uuidSet = false;
  for (uint8_t b : out.trayUuid) uuidSet |= (b != 0);
  return out.filamentType[0] != '\0' && uuidSet;
}

SpoolInfo toSpoolInfo(const TagData& tag) {
  SpoolInfo s;
  const std::string type = tag.filamentType;
  const std::string detailed = tag.detailedType;

  // "PLA Basic" -> material "PLA", subtype "Basic"; "PETG-HF" -> material "PETG-HF".
  s.material = type.empty() ? "PLA" : type;
  size_t space = detailed.find(' ');
  if (space != std::string::npos) {
    std::string head = detailed.substr(0, space);
    std::string a = head, b = s.material;
    for (char& c : a) c = (char)toupper((unsigned char)c);
    for (char& c : b) c = (char)toupper((unsigned char)c);
    if (a == b) {
      s.subtype = detailed.substr(space + 1);
    } else {
      s.material = detailed;
    }
  } else if (!detailed.empty()) {
    std::string a = detailed, b = s.material;
    for (char& c : a) c = (char)toupper((unsigned char)c);
    for (char& c : b) c = (char)toupper((unsigned char)c);
    if (a != b) s.material = detailed;
  }

  // Variant id "A00-M1": M* = gradient (dual colour on PLA Silk A05), T* = tri colour.
  const std::string variant = tag.variantId;
  size_t dash = variant.find('-');
  if (dash != std::string::npos && dash + 1 < variant.size()) {
    char code = variant[dash + 1];
    if (code == 'M') {
      s.subtype = variant.substr(0, dash) == "A05" ? "Dual Color" : "Gradient";
    } else if (code == 'T') {
      s.subtype = "Tri Color";
    }
  }

  s.rgba = toHex(tag.rgba, 4);
  if (tag.colorCount >= 2) s.extraColors = lowerHex(tag.secondaryRgba, 4);
  if (!s.subtype.empty()) s.effectType = effectFromSubtype(s.subtype);

  s.slicerFilament = tag.materialId;
  s.slicerFilamentName = detailed;
  s.detailedType = detailed;
  s.labelWeight = tag.weightGrams ? tag.weightGrams : 1000;
  s.nozzleTempMin = tag.nozzleMinC;
  s.nozzleTempMax = tag.nozzleMaxC;
  s.tagUid = toHex(tag.uid, 4);
  s.trayUuid = toHex(tag.trayUuid, 16);

  // "2025_01_16_12_09" -> "2025-01-16 12:09"
  std::string d = tag.productionDate;
  if (d.size() == 16 && d[4] == '_' && d[7] == '_' && d[10] == '_' && d[13] == '_') {
    s.productionDate = d.substr(0, 4) + "-" + d.substr(5, 2) + "-" + d.substr(8, 2) + " " +
                       d.substr(11, 2) + ":" + d.substr(14, 2);
  } else {
    s.productionDate = d;
  }
  return s;
}

}  // namespace bambu
