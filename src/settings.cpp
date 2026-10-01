#include "settings.h"

#include <ImprovWiFiLibrary.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "feedback.h"
#include "status.h"

namespace settings {

Settings current;

namespace {

const char* const NVS_NAMESPACE = "scanner";
const char* const PORTAL_SSID = "SpoolScanner-Setup";
const char* const HOSTNAME = "spool-scanner";
const uint32_t IMPROV_CONNECT_TIMEOUT_MS = 20000;

WiFiManager wm;
WiFiManagerParameter statusWidget(status::STATUS_WIDGET_HTML);
WiFiManagerParameter urlParam("url", "Bambuddy URL (e.g. http://192.168.1.50:8000)", "", 128);
WiFiManagerParameter keyParam("apikey", "Bambuddy API key (needs Manage inventory)", "", 96);
WiFiManagerParameter locParam("location", "Storage location for new spools (optional)", "", 64);
ImprovWiFi improv(&Serial);
bool announcedIp = false;
const char* firmware = "";

String normalizeUrl(String url) {
  url.trim();
  while (url.endsWith("/")) url.remove(url.length() - 1);
  if (url.length() && !url.startsWith("http://") && !url.startsWith("https://")) {
    url = "http://" + url;
  }
  return url;
}

void saveParams() {
  current.bambuddyUrl = normalizeUrl(urlParam.getValue());
  current.apiKey = String(keyParam.getValue());
  current.apiKey.trim();
  current.storageLocation = String(locParam.getValue());
  current.storageLocation.trim();

  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("url", current.bambuddyUrl);
  prefs.putString("apikey", current.apiKey);
  prefs.putString("location", current.storageLocation);
  prefs.end();
  Serial.printf("Settings saved, Bambuddy: %s\n", current.bambuddyUrl.c_str());
  status::requestCheck();
}

void addStatusRoute() {
  wm.server->on("/status.json", HTTP_GET, []() {
    if (wm.server->hasArg("refresh")) status::requestCheck();
    wm.server->sendHeader("Cache-Control", "no-store");
    wm.server->send(200, "application/json", status::json(firmware));
  });
}

// Called by Improv with the WiFi the user entered in the web installer.
bool improvConnect(const char* ssid, const char* password) {
  if (wm.getConfigPortalActive()) wm.stopConfigPortal();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);  // keep the credentials for the next boot
  WiFi.begin(ssid, password);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < IMPROV_CONNECT_TIMEOUT_MS) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

void improvConnected(const char* ssid, const char*) {
  Serial.printf("WiFi \"%s\" set up from the web installer\n", ssid);
}

}  // namespace

void begin(const char* firmwareVersion) {
  firmware = firmwareVersion;
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);
  current.bambuddyUrl = prefs.getString("url", "");
  current.apiKey = prefs.getString("apikey", "");
  current.storageLocation = prefs.getString("location", "");
  prefs.end();

  urlParam.setValue(current.bambuddyUrl.c_str(), 128);
  keyParam.setValue(current.apiKey.c_str(), 96);
  locParam.setValue(current.storageLocation.c_str(), 64);

  improv.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "Bambuddy Spool Scanner", firmwareVersion,
                       "Spool Scanner", "http://{LOCAL_IPV4}/param");
  improv.setCustomConnectWiFi(improvConnect);
  improv.onImprovConnected(improvConnected);

  WiFi.mode(WIFI_STA);
  wm.setHostname(HOSTNAME);
  wm.setTitle("Spool Scanner");
  wm.addParameter(&statusWidget);
  wm.addParameter(&urlParam);
  wm.addParameter(&keyParam);
  wm.addParameter(&locParam);
  wm.setSaveParamsCallback(saveParams);
  wm.setWebServerCallback(addStatusRoute);
  wm.setCustomMenuHTML(status::STATUS_WIDGET_HTML);
  std::vector<const char*> menu = {"custom", "param", "wifi", "info", "restart"};
  wm.setMenu(menu);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);
  wm.setConnectTimeout(15);

  // Connects with stored credentials, or opens the setup access point and
  // returns straight away; loop() keeps both it and Improv responsive.
  if (!wm.autoConnect(PORTAL_SSID)) {
    Serial.printf("No WiFi yet: use the web installer, or join \"%s\"\n", PORTAL_SSID);
  }
}

void loop() {
  improv.handleSerial();
  wm.process();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wm.getConfigPortalActive() && !wm.getWebPortalActive()) wm.startWebPortal();
    if (!announcedIp) {
      announcedIp = true;
      Serial.printf("WiFi connected. Settings page: http://%s/param\n",
                    WiFi.localIP().toString().c_str());
      if (current.bambuddyUrl.isEmpty()) Serial.println("Bambuddy URL not set yet");
    }
  } else {
    announcedIp = false;
  }
}

void openSetupPortal() {
  Serial.printf("Opening setup access point \"%s\"\n", PORTAL_SSID);
  feedback::portal();
  if (wm.getWebPortalActive()) wm.stopWebPortal();
  wm.startConfigPortal(PORTAL_SSID);
}

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }

}  // namespace settings
