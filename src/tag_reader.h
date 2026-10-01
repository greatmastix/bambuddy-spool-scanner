// Reads a Bambu Lab spool tag with an MFRC522 (RC522) module.
#pragma once

#include <Arduino.h>

#include "bambu_tag.h"

namespace tag_reader {

enum class ReadResult { NoTag, Ok, NotBambu, AuthFailed, ReadFailed };

// Returns false if the RC522 does not answer over SPI (wiring problem).
bool begin();
bool ok();

// Polls for a tag; on Ok, `out` holds the decoded tag. Signals feedback::reading()
// as soon as a MIFARE Classic tag is detected.
ReadResult poll(bambu::TagData& out);

const char* describe(ReadResult r);

}  // namespace tag_reader
