// Power: hold latch, PWR key, battery voltage (issue 25). Circuit notes in
// docs/pins.md ("Power").
#pragma once

#include <stdint.h>

enum class BootSource : uint8_t { PowerKey, Usb, Wake };

// Call first thing in setup(), before anything slow: on battery the board
// stays on only while PWR is held, until BAT_Control is driven high here.
void power_begin();

BootSource power_boot_source();

// Battery voltage in volts (BAT_ADC x 3, averaged). With USB connected and no
// cell fitted the charger output floats, so the value is meaningless then.
float power_battery_volts();

// Rough state of charge from the voltage (single-cell LiPo curve), 0..100.
int power_battery_percent(float volts);

enum class KeyEvent : uint8_t { None, Short, LongHeld };

// Poll every loop. Short: pressed and released in under LONG_MS. LongHeld:
// reported once when the key has been held for LONG_MS (still held).
KeyEvent power_key_poll();

// Drop the hold latch. On battery the board turns off as soon as PWR is
// released. If it is still running RELEASE_WAIT_MS after release (USB power),
// it goes to deep sleep, woken by the PWR key. Does not return.
[[noreturn]] void power_off();
