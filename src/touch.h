// SPD2010 touch controller on the shared I2C bus (issue 23).
// The display must be initialised first: the touch side of the SPD2010 does
// not answer on I2C until then (issue 22).
#pragma once

#include <stdint.h>

constexpr uint8_t TOUCH_ADDR = 0x53;
constexpr int TOUCH_MAX_POINTS = 10;

struct TouchPoint {
  uint8_t id;
  uint16_t x, y;   // panel coordinates, same as the display
  uint8_t weight;  // 0 on lift
};

// Resets the controller through the expander and reads its firmware info.
bool touch_begin();

// Runs one step of the controller's state machine and returns the number of
// points in this report (0 when nothing is touching or no report is ready).
// Call it regularly (every 10..20 ms): it reads the status register on every
// call, as the reference drivers do (issue 31 is about keying off INT).
int touch_read(TouchPoint *pts, int max_pts);

// True while a finger is on the panel: set by a point report, cleared by the
// lift report (weight 0). Stays set while a finger is held still.
bool touch_down();

// Count of INT falling edges seen since boot (diagnostic).
uint32_t touch_irq_count();
