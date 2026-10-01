// Persistent configuration (NVS), WiFi setup and the settings web page.
//
// WiFi can be set up three ways:
//  - from the web installer right after flashing (Improv Wi-Fi over serial),
//  - from the "SpoolScanner-Setup" access point that opens when no WiFi works,
//  - by holding BOOT for 3 s, which reopens that access point.
// Once on WiFi, the Bambuddy settings live at http://<scanner-ip>/param.
#pragma once

#include <Arduino.h>

struct Settings {
  String bambuddyUrl;      // e.g. "http://192.168.1.50:8000"
  String apiKey;           // Bambuddy API key with "Manage inventory", empty if auth is off
  String storageLocation;  // optional, written to new spools' storage_location
};

namespace settings {

extern Settings current;

// Loads settings and starts connecting to WiFi without blocking.
void begin(const char* firmwareVersion);
// Call from loop(): serves Improv, the setup access point and the settings page.
void loop();
// Opens the setup access point (also changes WiFi).
void openSetupPortal();
bool wifiConnected();

}  // namespace settings
