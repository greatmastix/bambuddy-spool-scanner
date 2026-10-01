// Minimal client for the Bambuddy inventory API (/api/v1/inventory/...).
#pragma once

#include <Arduino.h>

#include "bambu_tag.h"

namespace bambuddy {

enum class Result { Created, AlreadyExists, Error };

struct Outcome {
  Result result;
  int spoolId;     // Bambuddy spool id, -1 if unknown
  String message;  // human readable, for the serial log
};

// Looks the spool up by tray UUID / tag UID and creates it if Bambuddy does not know it yet.
Outcome addSpool(const bambu::SpoolInfo& spool);

// Checks that Bambuddy is reachable and accepts the API key, using a cheap
// by-tag lookup for a tag that cannot exist. Returns the HTTP status (404 means
// fine), or a negative transport error; `detail` gets a short description.
int ping(String& detail);

}  // namespace bambuddy
