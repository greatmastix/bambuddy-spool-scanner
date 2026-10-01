#include "settings.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "feedback.h"

namespace settings {

Settings current;

namespace {

const char* const NVS_NAMESPACE = "scanner";
const char* const PORTAL_SSID = "SpoolScanner-Setup";

void save() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("url", current.bambuddyUrl);
  prefs.putString("apikey", current.apiKey);
  prefs.putString("location", current.storageLocation);
  prefs.end();
}

String normalizeUrl(String url) {
  url.trim();
  while (url.endsWith("/")) url.remove(url.length() - 1);
  if (url.length() && !url.startsWith("http://") && !url.startsWith("https://")) {
    url = "http://" + url;
  }
  return url;
}

}  // namespace

void load() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);
  current.bambuddyUrl = prefs.getString("url", "");
  current.apiKey = prefs.getString("apikey", "");
  current.storageLocation = prefs.getString("location", "");
  prefs.end();
}

void connectWifi(bool forcePortal) {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  WiFiManagerParameter urlParam("url", "Bambuddy URL (e.g. http://192.168.1.50:8000)",
                                current.bambuddyUrl.c_str(), 128);
  WiFiManagerParameter keyParam("apikey", "Bambuddy API key (needs Manage inventory)",
                                current.apiKey.c_str(), 96);
  WiFiManagerParameter locParam("location", "Storage location for new spools (optional)",
                                current.storageLocation.c_str(), 64);
  wm.addParameter(&urlParam);
  wm.addParameter(&keyParam);
  wm.addParameter(&locParam);

  bool paramsChanged = false;
  wm.setSaveParamsCallback([&]() { paramsChanged = true; });
  wm.setSaveConfigCallback([&]() { paramsChanged = true; });
  // Show the Bambuddy fields on the WiFi page and offer a separate "Setup" page
  // so the URL/key can be changed without re-entering the WiFi password.
  std::vector<const char*> menu = {"wifi", "param", "info", "restart"};
  wm.setMenu(menu);
  wm.setConfigPortalTimeout(0);

  bool connected;
  if (forcePortal) {
    Serial.printf("Opening setup portal: join WiFi \"%s\"\n", PORTAL_SSID);
    feedback::portal();
    connected = wm.startConfigPortal(PORTAL_SSID);
  } else {
    if (current.bambuddyUrl.isEmpty()) Serial.println("No Bambuddy URL configured yet");
    connected = wm.autoConnect(PORTAL_SSID);
  }

  if (paramsChanged) {
    current.bambuddyUrl = normalizeUrl(urlParam.getValue());
    current.apiKey = String(keyParam.getValue());
    current.apiKey.trim();
    current.storageLocation = String(locParam.getValue());
    current.storageLocation.trim();
    save();
    Serial.println("Settings saved");
  }

  if (!connected) {
    Serial.println("WiFi not connected, restarting");
    delay(1000);
    ESP.restart();
  }
  Serial.printf("WiFi connected, IP %s\n", WiFi.localIP().toString().c_str());
}

}  // namespace settings
