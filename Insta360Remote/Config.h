#pragma once

// 1: ORBIT identity advertisement used by community X4 implementations.
// 0: advertise both GATT service UUIDs instead. Try 0 if not discovered.
// Neither option implements camera power-on / wakeup.
#define REMOTE_ORBIT_ADVERTISING 1

// Rotation 1/3 gives a 240 x 135 landscape display on StickS3.
#define REMOTE_DISPLAY_ROTATION 1
#define REMOTE_BRIGHTNESS 100

// Keep BLE connected and buttons active while the local backlight is off.
// Only button use and connection changes restart these timers; camera RX does not.
// Set either timeout to 0 to disable that stage.
#define REMOTE_DIM_BRIGHTNESS 15
#define REMOTE_SCREEN_DIM_AFTER_MS 6000UL
#define REMOTE_SCREEN_OFF_AFTER_MS 10000UL

// Conservative reduction from 240 MHz; APB remains at 80 MHz.
// Set back to 240 if you want the original CPU clock.
#define REMOTE_CPU_MHZ 160
