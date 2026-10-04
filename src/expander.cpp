#include "expander.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"

static uint8_t addr = 0;
static uint8_t out_latch = 0xFF;

static bool present(uint8_t a) {
  Wire.beginTransmission(a);
  return Wire.endTransmission() == 0;
}

static bool write_reg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

int expander_read(uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

uint8_t expander_addr() { return addr; }

void expander_dump(const char *when) {
  Serial.printf("tca9554 @0x%02x (%s): in 0x%02x out 0x%02x pol 0x%02x cfg 0x%02x\n",
                addr, when, expander_read(TCA_INPUT), expander_read(TCA_OUTPUT),
                expander_read(TCA_POLARITY), expander_read(TCA_CONFIG));
}

bool expander_begin() {
  for (uint8_t a = 0x20; a <= 0x27; a++)
    if (present(a)) { addr = a; break; }
  if (!addr) return false;
  // Latch high before switching to output, so the resets do not glitch low.
  out_latch = 0xFF;
  write_reg(TCA_OUTPUT, out_latch);
  const uint8_t outs = (1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST) | (1 << EXIO_SD_CS);
  write_reg(TCA_CONFIG, (uint8_t)~outs);
  return true;
}

void expander_set(uint8_t bit, bool high) {
  if (high) out_latch |= (1 << bit);
  else out_latch &= ~(1 << bit);
  write_reg(TCA_OUTPUT, out_latch);
}
