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

void reading() {
  digitalWrite(PIN_LED, HIGH);
  if (PIN_BUZZER >= 0) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(30);
    digitalWrite(PIN_BUZZER, LOW);
  }
}

void done() { set(false); }

void tagLost() {
  set(false);
  delay(150);
  blink(3, 80, 80);
}

void added() {
  set(false);
  delay(150);
  blink(1, 800, 200);
}
void alreadyKnown() {
  set(false);
  delay(150);
  blink(2, 120, 150);
}
void error() {
  set(false);
  delay(150);
  blink(5, 60, 80);
}
void portal() { blink(3, 300, 300); }

}  // namespace feedback
