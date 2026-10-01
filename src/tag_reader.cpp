#include "tag_reader.h"

#include <MFRC522.h>
#include <SPI.h>

#include "pins.h"

namespace tag_reader {

namespace {

MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);

void release() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

}  // namespace

bool begin() {
  SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
  rfid.PCD_Init();
  delay(10);
  const byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("RC522 firmware version 0x%02X\n", version);
  if (version == 0x00 || version == 0xFF) return false;
  // Maximum receiver gain buys a few millimetres, which matters through a carton.
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  return true;
}

ReadResult poll(bambu::TagData& out) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return ReadResult::NoTag;

  const MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
  if (type != MFRC522::PICC_TYPE_MIFARE_1K || rfid.uid.size != 4) {
    release();
    return ReadResult::NotBambu;
  }

  uint8_t keys[bambu::SECTOR_COUNT][bambu::KEY_LEN];
  bambu::deriveSectorKeys(rfid.uid.uidByte, rfid.uid.size, keys);

  uint8_t blocks[bambu::SECTORS_TO_READ * 4][bambu::BLOCK_LEN];
  for (uint8_t sector = 0; sector < bambu::SECTORS_TO_READ; sector++) {
    MFRC522::MIFARE_Key key;
    memcpy(key.keyByte, keys[sector], bambu::KEY_LEN);
    const uint8_t trailer = sector * 4 + 3;
    if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailer, &key, &rfid.uid) !=
        MFRC522::STATUS_OK) {
      release();
      return ReadResult::AuthFailed;
    }
    for (uint8_t i = 0; i < 3; i++) {
      const uint8_t block = sector * 4 + i;
      uint8_t buffer[18];
      uint8_t size = sizeof(buffer);
      if (rfid.MIFARE_Read(block, buffer, &size) != MFRC522::STATUS_OK) {
        release();
        return ReadResult::ReadFailed;
      }
      memcpy(blocks[block], buffer, bambu::BLOCK_LEN);
    }
    memset(blocks[trailer], 0, bambu::BLOCK_LEN);
  }
  release();

  return bambu::decodeTag(blocks, out) ? ReadResult::Ok : ReadResult::NotBambu;
}

const char* describe(ReadResult r) {
  switch (r) {
    case ReadResult::NoTag: return "no tag";
    case ReadResult::Ok: return "ok";
    case ReadResult::NotBambu: return "not a Bambu Lab spool tag";
    case ReadResult::AuthFailed: return "authentication failed (moved too fast or not a Bambu tag)";
    case ReadResult::ReadFailed: return "read failed (tag moved out of range?)";
  }
  return "?";
}

}  // namespace tag_reader
