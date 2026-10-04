// TCA9554 I/O expander on the shared I2C bus (issue 21). Port bits are the
// EXIO_* constants in board_pins.h.
#pragma once

#include <stdint.h>

// Finds the expander in 0x20..0x27, makes TP_RST, LCD_RST and SD_CS outputs
// idling high. Returns false if no expander answers. Wire must be started.
bool expander_begin();
uint8_t expander_addr();             // 0 if not found
void expander_set(uint8_t bit, bool high);
int expander_read(uint8_t reg);      // raw register, -1 on I2C error
void expander_dump(const char *when);

// Registers.
enum : uint8_t { TCA_INPUT = 0, TCA_OUTPUT = 1, TCA_POLARITY = 2, TCA_CONFIG = 3 };
