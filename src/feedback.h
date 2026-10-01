// LED / buzzer patterns so the scanner can be used without a screen.
#pragma once

namespace feedback {

void begin();
void busy(bool on);   // LED on while a tag is being processed
void added();         // one long blink: new spool created
void alreadyKnown();  // two short blinks: spool is already in Bambuddy
void error();         // five fast blinks: read or network error
void portal();        // three slow blinks: setup portal is open

}  // namespace feedback
