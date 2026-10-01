// Self-update from the firmware published on GitHub Pages by the web flasher workflow.
#pragma once

#include <Arduino.h>

namespace ota {

#ifndef OTA_BASE_URL
#define OTA_BASE_URL "https://greatmastix.github.io/bambuddy-spool-scanner/"
#endif

void begin(const char* currentVersion);
// Call from loop(): checks for a new version at boot and every few hours.
void loop();
void requestCheck();
// Starts downloading and installing the latest version in the background.
// Returns false if no update is known or one is already running.
bool startInstall();

// Fills the "update" object of /status.json.
void describe(String& state, String& latest, int& progressPct, String& detail);

}  // namespace ota
