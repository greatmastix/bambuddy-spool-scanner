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

}  // namespace bambuddy
