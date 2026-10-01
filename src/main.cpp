// Bambuddy Spool Scanner
//
// Hold a Bambu Lab spool (or its unopened box) over the RC522. The firmware
// derives the tag's keys from its UID, reads the spool data and adds the spool
// to Bambuddy's inventory unless it is already there.
#include <Arduino.h>

#include "bambuddy_client.h"
#include "feedback.h"
#include "ota.h"
#include "pins.h"
#include "settings.h"
#include "status.h"
#include "tag_reader.h"

namespace {

// Both tags of a spool share the tray UUID, and a box held over the reader is
// read many times a second: ignore the same spool for this long.
const uint32_t SAME_SPOOL_COOLDOWN_MS = 15000;
const uint32_t SETUP_HOLD_MS = 3000;

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev"
#endif

std::string lastTrayUuid;
uint32_t lastSpoolAt = 0;
uint32_t buttonDownSince = 0;

void logSpool(const bambu::TagData& tag, const bambu::SpoolInfo& s) {
  Serial.printf("Spool: %s %s | colour #%s | %d g | %.2f mm | nozzle %d-%d C\n",
                s.material.c_str(), s.subtype.c_str(), s.rgba.c_str(), s.labelWeight,
                tag.diameterMm, s.nozzleTempMin, s.nozzleTempMax);
  Serial.printf("       %s / %s | tag %s | tray %s | made %s\n", tag.materialId, tag.variantId,
                s.tagUid.c_str(), s.trayUuid.c_str(), s.productionDate.c_str());
}

void checkSetupButton() {
  if (digitalRead(PIN_SETUP_BUTTON) == LOW) {
    if (buttonDownSince == 0) buttonDownSince = millis();
    if (millis() - buttonDownSince >= SETUP_HOLD_MS) {
      buttonDownSince = 0;
      settings::openSetupPortal();
    }
  } else {
    buttonDownSince = 0;
  }
}

void handleTag(const bambu::TagData& tag) {
  const bambu::SpoolInfo spool = bambu::toSpoolInfo(tag);
  if (spool.trayUuid == lastTrayUuid && millis() - lastSpoolAt < SAME_SPOOL_COOLDOWN_MS) {
    feedback::done();
    return;
  }
  lastTrayUuid = spool.trayUuid;
  lastSpoolAt = millis();

  logSpool(tag, spool);
  const String name = String(spool.material.c_str()) + " " + spool.subtype.c_str() + " #" +
                      spool.rgba.substr(0, 6).c_str();

  if (!settings::wifiConnected()) {
    Serial.println("Not on WiFi yet, can't reach Bambuddy");
    status::reportScan(name + ": not on WiFi");
    feedback::error();
    lastTrayUuid.clear();
    return;
  }

  const bambuddy::Outcome outcome = bambuddy::addSpool(spool);
  switch (outcome.result) {
    case bambuddy::Result::Created:
      Serial.printf("Bambuddy: created spool #%d\n", outcome.spoolId);
      status::reportApi(status::Api::Connected, "connected");
      status::reportScan(name + ": added as spool #" + outcome.spoolId);
      feedback::added();
      break;
    case bambuddy::Result::AlreadyExists:
      Serial.printf("Bambuddy: spool #%d already in inventory\n", outcome.spoolId);
      status::reportApi(status::Api::Connected, "connected");
      status::reportScan(name + ": already in Bambuddy as spool #" + outcome.spoolId);
      feedback::alreadyKnown();
      break;
    case bambuddy::Result::Error:
      Serial.printf("Bambuddy: %s\n", outcome.message.c_str());
      status::reportScan(name + ": " + outcome.message);
      status::requestCheck();
      feedback::error();
      // Let the next read retry straight away.
      lastTrayUuid.clear();
      break;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nBambuddy Spool Scanner");

  feedback::begin();
  pinMode(PIN_SETUP_BUTTON, INPUT_PULLUP);

  Serial.printf("Firmware %s\n", FIRMWARE_VERSION);
  settings::begin(FIRMWARE_VERSION);
  ota::begin(FIRMWARE_VERSION);

  if (!tag_reader::begin()) {
    Serial.println("RC522 not responding, check wiring (see docs/wiring.md)");
    feedback::error();
  }
  Serial.println("Ready. Hold a spool or its box over the reader.");
}

void loop() {
  settings::loop();
  status::loop();
  ota::loop();
  checkSetupButton();

  bambu::TagData tag;
  const tag_reader::ReadResult r = tag_reader::poll(tag);
  if (r == tag_reader::ReadResult::Ok) {
    handleTag(tag);
  } else if (r == tag_reader::ReadResult::NotBambu) {
    feedback::done();
    Serial.printf("Tag: %s\n", tag_reader::describe(r));
  } else if (r != tag_reader::ReadResult::NoTag) {
    // Detected but lost mid-read: tell the user to hold still and try again.
    feedback::tagLost();
    status::reportScan(String("read interrupted: ") + tag_reader::describe(r));
    Serial.printf("Tag: %s\n", tag_reader::describe(r));
  }
  delay(50);
}
