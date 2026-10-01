// Live status for the scanner's web page: Bambuddy connection, reader, last scan.
#pragma once

#include <Arduino.h>

namespace status {

enum class Api { NotConfigured, Checking, Connected, AuthFailed, Unreachable, Error };

// Call from loop(): re-checks Bambuddy every minute, or soon after requestCheck().
void loop();
// Asks for a fresh Bambuddy check (settings saved, "Check now" button).
void requestCheck();
// Records the outcome of a real API call so the page does not wait for the next check.
void reportApi(Api api, const String& detail);
void reportScan(const String& text);

// JSON for /status.json.
String json(const char* firmwareVersion);

// HTML snippet that shows the status and refreshes it every few seconds.
extern const char STATUS_WIDGET_HTML[];

}  // namespace status
