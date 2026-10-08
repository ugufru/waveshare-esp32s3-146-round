// PCF85063 real-time clock on the shared I2C bus at 0x51 (issue 26).
// The RTC holds UTC; the local UTC offset is kept in its one RAM byte (in
// 15-minute steps) so both survive together. The RTC runs from 3V3 through a
// diode, with J2 for an optional backup cell (not fitted): without one, time
// is kept only while the board has power (always on USB, also in deep sleep).
#pragma once

#include <stdint.h>
#include <time.h>

// Reads the RTC. If its time is valid, sets the system clock (UTC) from it.
// Returns true when the time is valid; false when the oscillator stopped
// since it was last set (power was lost) or on I2C error.
bool rtc_begin();

// Writes UTC and the local offset (minutes, multiple of 15) to the RTC and
// sets the system clock. Clears the oscillator-stop flag.
bool rtc_set(time_t utc, int offset_min);

// Local offset in minutes, as stored in the RTC.
int rtc_offset_min();

bool rtc_valid();
