#include "tag_reader.h"

#include <MFRC522.h>
#include <SPI.h>

#include "feedback.h"
#include "pins.h"

namespace tag_reader {

namespace {

// A tag at the edge of the field (e.g. through a box) can drop out mid-read.
// Re-select it and start over a few times before giving up.
const int READ_ATTEMPTS = 4;
const uint32_t RETRY_DELAY_MS = 40;

MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
bool readerOk = false;

void release() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

ReadResult readBlocks(const uint8_t keys[bambu::SECTOR_COUNT][bambu::KEY_LEN],
                      uint8_t blocks[bambu::SECTORS_TO_READ * 4][bambu::BLOCK_LEN]) {
  for (uint8_t sector = 0; sector < bambu::SECTORS_TO_READ; sector++) {
    MFRC522::MIFARE_Key key;
    memcpy(key.keyByte, keys[sector], bambu::KEY_LEN);
    const uint8_t trailer = sector * 4 + 3;
    if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailer, &key, &rfid.uid) !=
        MFRC522::STATUS_OK) {
      return ReadResult::AuthFailed;
    }
    for (uint8_t i = 0; i < 3; i++) {
      const uint8_t block = sector * 4 + i;
      uint8_t buffer[18];
      uint8_t size = sizeof(buffer);
      if (rfid.MIFARE_Read(block, buffer, &size) != MFRC522::STATUS_OK) return ReadResult::ReadFailed;
      memcpy(blocks[block], buffer, bambu::BLOCK_LEN);
    }
    memset(blocks[trailer], 0, bambu::BLOCK_LEN);
  }
  return ReadResult::Ok;
}

// Wakes and re-selects the same tag after a failed attempt.
bool reselect(const uint8_t uid[4]) {
  rfid.PCD_StopCrypto1();
  uint8_t atqa[2];
  uint8_t atqaSize = sizeof(atqa);
  if (rfid.PICC_WakeupA(atqa, &atqaSize) != MFRC522::STATUS_OK) return false;
  if (rfid.PICC_Select(&rfid.uid) != MFRC522::STATUS_OK) return false;
  return rfid.uid.size == 4 && memcmp(rfid.uid.uidByte, uid, 4) == 0;
}

}  // namespace

bool begin() {
  SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
  rfid.PCD_Init();
  delay(10);
  const byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("RC522 firmware version 0x%02X\n", version);
  readerOk = !(version == 0x00 || version == 0xFF);
  // Maximum receiver gain buys a few millimetres, which matters through a carton.
  if (readerOk) rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  return readerOk;
}

bool ok() { return readerOk; }

ReadResult poll(bambu::TagData& out) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return ReadResult::NoTag;

  const MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
  if (type != MFRC522::PICC_TYPE_MIFARE_1K || rfid.uid.size != 4) {
    release();
    return ReadResult::NotBambu;
  }

  feedback::reading();

  uint8_t uid[4];
  memcpy(uid, rfid.uid.uidByte, 4);
  uint8_t keys[bambu::SECTOR_COUNT][bambu::KEY_LEN];
  bambu::deriveSectorKeys(uid, 4, keys);

  uint8_t blocks[bambu::SECTORS_TO_READ * 4][bambu::BLOCK_LEN];
  ReadResult result = ReadResult::ReadFailed;
  for (int attempt = 0; attempt < READ_ATTEMPTS; attempt++) {
    if (attempt > 0) {
      delay(RETRY_DELAY_MS);
      if (!reselect(uid)) continue;
    }
    result = readBlocks(keys, blocks);
    if (result == ReadResult::Ok) break;
  }
  release();

  if (result != ReadResult::Ok) return result;
  return bambu::decodeTag(blocks, out) ? ReadResult::Ok : ReadResult::NotBambu;
}

const char* describe(ReadResult r) {
  switch (r) {
    case ReadResult::NoTag: return "no tag";
    case ReadResult::Ok: return "ok";
    case ReadResult::NotBambu: return "not a Bambu Lab spool tag";
    case ReadResult::AuthFailed: return "authentication failed (tag moved away, or not a Bambu tag)";
    case ReadResult::ReadFailed: return "read failed (tag moved out of range)";
  }
  return "?";
}

}  // namespace tag_reader
