#include "ota.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include "ota_roots.h"
#include "settings.h"

namespace ota {

namespace {

const uint32_t CHECK_INTERVAL_MS = 6UL * 60 * 60 * 1000;
const uint32_t TIMEOUT_MS = 15000;

enum class State { Idle, Checking, UpToDate, Available, Installing, Done, Failed };

const char* current = "";
volatile State state = State::Idle;
volatile int progress = 0;
String latestVersion;
String binUrl;
String binMd5;
int binSize = 0;
String detail;
bool checkRequested = true;
uint32_t lastCheckAt = 0;

void configure(WiFiClientSecure& client) {
  client.setCACert(OTA_ROOT_CAS);
  client.setTimeout(TIMEOUT_MS / 1000);
}

void check() {
  checkRequested = false;
  lastCheckAt = millis();
  state = State::Checking;

  WiFiClientSecure client;
  configure(client);
  HTTPClient http;
  http.setTimeout(TIMEOUT_MS);
  if (!http.begin(client, String(OTA_BASE_URL) + "ota.json")) {
    state = State::Failed;
    detail = "could not start update check";
    return;
  }
  const int code = http.GET();
  if (code != 200) {
    state = State::Failed;
    detail = "update check failed: " + (code > 0 ? "HTTP " + String(code) : HTTPClient::errorToString(code));
    http.end();
    return;
  }
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err || !doc["version"].is<const char*>() || !doc["path"].is<const char*>()) {
    state = State::Failed;
    detail = "update check failed: bad ota.json";
    return;
  }
  latestVersion = doc["version"].as<const char*>();
  binUrl = String(OTA_BASE_URL) + doc["path"].as<const char*>();
  binMd5 = doc["md5"] | "";
  binSize = doc["size"] | 0;
  detail = "";
  state = latestVersion == current ? State::UpToDate : State::Available;
  Serial.printf("Update check: running %s, latest %s\n", current, latestVersion.c_str());
}

void installTask(void*) {
  WiFiClientSecure client;
  configure(client);
  HTTPClient http;
  http.setTimeout(TIMEOUT_MS);
  bool ok = false;
  if (http.begin(client, binUrl)) {
    const int code = http.GET();
    const int length = http.getSize();
    if (code != 200) {
      detail = "download failed: " + (code > 0 ? "HTTP " + String(code) : HTTPClient::errorToString(code));
    } else if (length <= 0 || (binSize && length != binSize)) {
      detail = "download failed: unexpected size";
    } else if (!Update.begin(length)) {
      detail = String("not enough space: ") + Update.errorString();
    } else {
      if (binMd5.length() == 32) Update.setMD5(binMd5.c_str());
      WiFiClient* stream = http.getStreamPtr();
      uint8_t buf[2048];
      int written = 0;
      uint32_t lastData = millis();
      while (written < length && millis() - lastData < TIMEOUT_MS) {
        const size_t avail = stream->available();
        if (!avail) {
          delay(5);
          continue;
        }
        const int n = stream->readBytes(buf, min(avail, sizeof(buf)));
        if (n <= 0) continue;
        if (Update.write(buf, n) != (size_t)n) break;
        written += n;
        lastData = millis();
        progress = (int)(100LL * written / length);
      }
      if (written != length) {
        Update.abort();
        detail = "download interrupted";
      } else if (!Update.end()) {
        detail = String("verification failed: ") + Update.errorString();
      } else {
        ok = true;
      }
    }
    http.end();
  } else {
    detail = "could not start download";
  }

  if (ok) {
    state = State::Done;
    detail = "installed " + latestVersion + ", restarting";
    Serial.println("Update installed, restarting");
    delay(1500);
    ESP.restart();
  }
  Serial.printf("Update failed: %s\n", detail.c_str());
  state = State::Failed;
  vTaskDelete(nullptr);
}

}  // namespace

void begin(const char* currentVersion) { current = currentVersion; }

void loop() {
  if (state == State::Installing || !settings::wifiConnected()) return;
  if (checkRequested || lastCheckAt == 0 || millis() - lastCheckAt >= CHECK_INTERVAL_MS) check();
}

void requestCheck() {
  if (state != State::Installing) checkRequested = true;
}

bool startInstall() {
  if (state != State::Available && !(state == State::Failed && binUrl.length())) return false;
  state = State::Installing;
  progress = 0;
  detail = "";
  Serial.printf("Installing update %s\n", latestVersion.c_str());
  // TLS needs a deep stack; run in the background so the web page can show progress.
  if (xTaskCreate(installTask, "ota", 16384, nullptr, 1, nullptr) != pdPASS) {
    state = State::Failed;
    detail = "could not start update task";
    return false;
  }
  return true;
}

void describe(String& s, String& latest, int& pct, String& d) {
  switch (state) {
    case State::Idle: s = "idle"; break;
    case State::Checking: s = "checking"; break;
    case State::UpToDate: s = "up_to_date"; break;
    case State::Available: s = "available"; break;
    case State::Installing: s = "installing"; break;
    case State::Done: s = "done"; break;
    case State::Failed: s = "failed"; break;
  }
  latest = latestVersion;
  pct = progress;
  d = detail;
}

}  // namespace ota
