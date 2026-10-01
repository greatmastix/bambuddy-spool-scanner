#include "feedback.h"

#include <Arduino.h>

#include "pins.h"

namespace feedback {

namespace {

void set(bool on) {
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  if (PIN_BUZZER >= 0) digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

void blink(int count, int onMs, int offMs) {
  for (int i = 0; i < count; i++) {
    set(true);
    delay(onMs);
    set(false);
    delay(offMs);
  }
}

}  // namespace

void begin() {
  pinMode(PIN_LED, OUTPUT);
  if (PIN_BUZZER >= 0) pinMode(PIN_BUZZER, OUTPUT);
  set(false);
}

void busy(bool on) { digitalWrite(PIN_LED, on ? HIGH : LOW); }
void added() { blink(1, 800, 200); }
void alreadyKnown() { blink(2, 120, 150); }
void error() { blink(5, 60, 80); }
void portal() { blink(3, 300, 300); }

}  // namespace feedback
