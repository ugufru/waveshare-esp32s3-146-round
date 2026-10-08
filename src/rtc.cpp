// Registers from the PCF85063A datasheet: 0x00 Control_1, 0x04..0x0A
// seconds, minutes, hours, days, weekdays, months, years (BCD). Seconds bit 7
// is OS, the oscillator-stop flag. The local offset is kept in NVS, not in
// the RTC's RAM byte: without a backup cell the RTC loses its RAM along with
// the time on power loss, and NTP then restored the time with offset 0.
#include "rtc.h"

#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <sys/time.h>

constexpr uint8_t ADDR = 0x51;
enum : uint8_t { CONTROL_1 = 0x00, SECONDS = 0x04 };

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

static void save_offset(int off) {
  Preferences p;
  p.begin("time", false);
  p.putShort("offset_min", off);
  p.end();
}

bool rtc_begin() {
  Preferences p;
  p.begin("time", true);
  offset_min = p.getShort("offset_min", 0);
  p.end();

  uint8_t c1, t[7];
  if (!rd(CONTROL_1, &c1, 1) || !rd(SECONDS, t, 7)) {
    Serial.println("rtc: no answer at 0x51");
    return false;
  }
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
  Wire.write(SECONDS);
  Wire.write(bin2bcd(tmv.tm_sec));  // OS bit 0: clears the stop flag
  Wire.write(bin2bcd(tmv.tm_min));
  Wire.write(bin2bcd(tmv.tm_hour));
  Wire.write(bin2bcd(tmv.tm_mday));
  Wire.write(tmv.tm_wday);
  Wire.write(bin2bcd(tmv.tm_mon + 1));
  Wire.write(bin2bcd(tmv.tm_year - 100));
  if (Wire.endTransmission() != 0) return false;
  if (off != offset_min) save_offset(off);
  offset_min = off;
  valid = true;
  set_system_clock(utc);
  return true;
}

int rtc_offset_min() { return offset_min; }
bool rtc_valid() { return valid; }
