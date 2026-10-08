// Registers from the PCF85063A datasheet: 0x00 Control_1, 0x03 RAM_byte,
// 0x04..0x0A seconds, minutes, hours, days, weekdays, months, years (BCD).
// Seconds bit 7 is OS, the oscillator-stop flag.
#include "rtc.h"

#include <Arduino.h>
#include <Wire.h>
#include <sys/time.h>

constexpr uint8_t ADDR = 0x51;
enum : uint8_t { CONTROL_1 = 0x00, RAM_BYTE = 0x03, SECONDS = 0x04 };

static int offset_min = 0;
static bool valid = false;

static uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t bin2bcd(uint8_t v) { return (v / 10) << 4 | (v % 10); }

static bool rd(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(ADDR, len) != len) return false;
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

static void set_system_clock(time_t utc) {
  timeval tv = {utc, 0};
  settimeofday(&tv, nullptr);
}

bool rtc_begin() {
  uint8_t c1, ram, t[7];
  if (!rd(CONTROL_1, &c1, 1) || !rd(RAM_BYTE, &ram, 1) || !rd(SECONDS, t, 7)) {
    Serial.println("rtc: no answer at 0x51");
    return false;
  }
  offset_min = (int8_t)ram * 15;
  valid = !(t[0] & 0x80) && !(c1 & 0x20);  // OS clear and clock not stopped
  tm tmv = {};
  tmv.tm_sec = bcd2bin(t[0] & 0x7F);
  tmv.tm_min = bcd2bin(t[1] & 0x7F);
  tmv.tm_hour = bcd2bin(t[2] & 0x3F);  // 24-hour mode (Control_1 bit 1 = 0)
  tmv.tm_mday = bcd2bin(t[3] & 0x3F);
  tmv.tm_mon = bcd2bin(t[5] & 0x1F) - 1;
  tmv.tm_year = bcd2bin(t[6]) + 100;  // 2000..2099
  // The RTC holds UTC; mktime would apply TZ, so stay in UTC (TZ unset).
  const time_t utc = mktime(&tmv);
  Serial.printf("rtc: control_1 0x%02x, %04d-%02d-%02d %02d:%02d:%02d UTC, offset %+d min, %s\n",
                c1, tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min,
                tmv.tm_sec, offset_min, valid ? "valid" : "INVALID (oscillator stopped: power was lost)");
  if (valid) set_system_clock(utc);
  return valid;
}

bool rtc_set(time_t utc, int off) {
  tm tmv;
  gmtime_r(&utc, &tmv);
  Wire.beginTransmission(ADDR);
  Wire.write(RAM_BYTE);
  Wire.write((uint8_t)(int8_t)(off / 15));
  Wire.write(bin2bcd(tmv.tm_sec));  // OS bit 0: clears the stop flag
  Wire.write(bin2bcd(tmv.tm_min));
  Wire.write(bin2bcd(tmv.tm_hour));
  Wire.write(bin2bcd(tmv.tm_mday));
  Wire.write(tmv.tm_wday);
  Wire.write(bin2bcd(tmv.tm_mon + 1));
  Wire.write(bin2bcd(tmv.tm_year - 100));
  if (Wire.endTransmission() != 0) return false;
  offset_min = off;
  valid = true;
  set_system_clock(utc);
  return true;
}

int rtc_offset_min() { return offset_min; }
bool rtc_valid() { return valid; }
