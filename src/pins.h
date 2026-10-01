// Pin assignment for a generic ESP32 DevKit (ESP32-WROOM-32).
// Every pin can be overridden from platformio.ini build_flags, e.g. -DPIN_BUZZER=-1.
#pragma once

// RC522 on the VSPI bus
#ifndef PIN_RC522_SS
#define PIN_RC522_SS 5    // RC522 "SDA"
#endif
#ifndef PIN_RC522_SCK
#define PIN_RC522_SCK 18
#endif
#ifndef PIN_RC522_MOSI
#define PIN_RC522_MOSI 23
#endif
#ifndef PIN_RC522_MISO
#define PIN_RC522_MISO 19
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
