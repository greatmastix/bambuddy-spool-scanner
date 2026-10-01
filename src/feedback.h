// LED / buzzer patterns so the scanner can be used without a screen.
#pragma once

namespace feedback {

void begin();
void reading();       // tag detected: LED on + short chirp, "hold still"
void done();          // LED off without a pattern
void tagLost();       // three quick blinks: tag moved away mid-read, hold it still and retry
void added();         // one long blink: new spool created
void alreadyKnown();  // two short blinks: spool is already in Bambuddy
void error();         // five fast blinks: read or network error
void portal();        // three slow blinks: setup portal is open

}  // namespace feedback
