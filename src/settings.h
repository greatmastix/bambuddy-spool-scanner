// Persistent configuration (NVS) and the WiFiManager setup portal.
#pragma once

#include <Arduino.h>

struct Settings {
  String bambuddyUrl;      // e.g. "http://192.168.1.50:8000"
  String apiKey;           // Bambuddy API key with "Manage inventory", empty if auth is off
  String storageLocation;  // optional, written to new spools' storage_location
};

namespace settings {

extern Settings current;

void load();
// Connects to WiFi, opening the "SpoolScanner-Setup" portal when no WiFi is
// stored, the connection fails, or forcePortal is set. Blocks until connected.
void connectWifi(bool forcePortal);

}  // namespace settings
