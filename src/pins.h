// Pin assignment for a generic ESP32 DevKit (ESP32-WROOM-32).
// Every pin can be overridden from platformio.ini build_flags, e.g. -DPIN_BUZZER=-1.
#pragma once

// RC522 on the VSPI bus, routed through the GPIO matrix.
// Chosen so the RC522's header (SDA SCK MOSI MISO IRQ GND RST 3.3V) lines up
// 1:1 with the DevKit row 16 17 5 18 19 21 22 23. GPIO 19, 21 and 23 sit under
// IRQ, GND and 3.3V and must stay unused: the firmware never drives them.
#ifndef PIN_RC522_SS
#define PIN_RC522_SS 16    // RC522 "SDA"
#endif
#ifndef PIN_RC522_SCK
#define PIN_RC522_SCK 17
#endif
#ifndef PIN_RC522_MOSI
#define PIN_RC522_MOSI 5
#endif
#ifndef PIN_RC522_MISO
#define PIN_RC522_MISO 18
#endif
#ifndef PIN_RC522_RST
#define PIN_RC522_RST 22
#endif

// Status LED. GPIO2 is the blue on-board LED on most DevKits.
#ifndef PIN_LED
#define PIN_LED 2
#endif

// Optional active piezo buzzer. Driving the pin with nothing attached is harmless;
// set -1 to free the pin for something else.
#ifndef PIN_BUZZER
#define PIN_BUZZER 25
#endif

// Hold for 3 s while running to open the setup portal. GPIO0 is the BOOT button.
#ifndef PIN_SETUP_BUTTON
#define PIN_SETUP_BUTTON 0
#endif
