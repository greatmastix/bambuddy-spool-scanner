#include "bambuddy_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <memory>

#include "settings.h"

namespace bambuddy {

namespace {

const uint32_t HTTP_TIMEOUT_MS = 8000;

String urlEncode(const String& s) {
  static const char hex[] = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0f];
      out += hex[c & 0x0f];
    }
  }
  return out;
}

// Sends one request. Returns the HTTP status (negative on transport errors).
int request(const char* method, const String& path, const String& body, String& response) {
  const String url = settings::current.bambuddyUrl + "/api/v1" + path;

  std::unique_ptr<WiFiClient> client;
  if (url.startsWith("https://")) {
    auto* secure = new WiFiClientSecure();
    // Bambuddy is usually on the LAN with a self-signed certificate.
    secure->setInsecure();
    client.reset(secure);
  } else {
    client.reset(new WiFiClient());
  }

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(*client, url)) return -1;
  http.addHeader("Accept", "application/json");
  if (settings::current.apiKey.length()) http.addHeader("X-API-Key", settings::current.apiKey);

  int status;
  if (strcmp(method, "POST") == 0) {
    http.addHeader("Content-Type", "application/json");
    status = http.POST(body);
  } else {
    status = http.GET();
  }
  response = status > 0 ? http.getString() : HTTPClient::errorToString(status);
  http.end();
  return status;
}

int spoolIdFrom(const String& json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return -1;
  return doc["id"] | -1;
}

// Bambu's catalog name for the colour, e.g. "Cobalt Blue". Empty if unknown.
String lookupColorName(const bambu::SpoolInfo& spool) {
  String path = "/inventory/colors/by-material?hex=" + String(spool.rgba.substr(0, 6).c_str());
  if (!spool.detailedType.empty()) path += "&material=" + urlEncode(spool.detailedType.c_str());
  String response;
  if (request("GET", path, "", response) != 200) return "";
  JsonDocument doc;
  if (deserializeJson(doc, response)) return "";
  return doc["color_name"] | "";
}

}  // namespace

Outcome addSpool(const bambu::SpoolInfo& spool) {
  if (settings::current.bambuddyUrl.isEmpty()) {
    return {Result::Error, -1, "Bambuddy URL not configured"};
  }

  String response;
  String path = "/inventory/spools/by-tag?tray_uuid=" + String(spool.trayUuid.c_str()) +
                "&tag_uid=" + String(spool.tagUid.c_str());
  int status = request("GET", path, "", response);
  if (status == 200) {
    return {Result::AlreadyExists, spoolIdFrom(response), "already in inventory"};
  }
  if (status != 404) {
    return {Result::Error, -1, "lookup failed: HTTP " + String(status) + " " + response};
  }

  JsonDocument doc;
  doc["material"] = spool.material;
  if (!spool.subtype.empty()) doc["subtype"] = spool.subtype;
  doc["brand"] = "Bambu Lab";
  doc["rgba"] = spool.rgba;
  String colorName = lookupColorName(spool);
  if (colorName.length()) doc["color_name"] = colorName;
  if (!spool.extraColors.empty()) doc["extra_colors"] = spool.extraColors;
  if (!spool.effectType.empty()) doc["effect_type"] = spool.effectType;
  doc["label_weight"] = spool.labelWeight;
  if (!spool.slicerFilament.empty()) doc["slicer_filament"] = spool.slicerFilament;
  if (!spool.slicerFilamentName.empty()) doc["slicer_filament_name"] = spool.slicerFilamentName;
  if (spool.nozzleTempMin) doc["nozzle_temp_min"] = spool.nozzleTempMin;
  if (spool.nozzleTempMax) doc["nozzle_temp_max"] = spool.nozzleTempMax;
  doc["tag_uid"] = spool.tagUid;
  doc["tray_uuid"] = spool.trayUuid;
  doc["tag_type"] = "bambulab";
  doc["data_origin"] = "rfid_scanner";
  if (settings::current.storageLocation.length()) {
    doc["storage_location"] = settings::current.storageLocation;
  }
  if (!spool.productionDate.empty()) {
    doc["note"] = String("Added by spool scanner. Produced ") + spool.productionDate.c_str();
  }

  String body;
  serializeJson(doc, body);
  status = request("POST", "/inventory/spools", body, response);
  if (status == 200 || status == 201) {
    return {Result::Created, spoolIdFrom(response), "created"};
  }
  return {Result::Error, -1, "create failed: HTTP " + String(status) + " " + response};
}

}  // namespace bambuddy
