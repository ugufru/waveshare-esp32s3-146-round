// Register values from the QMI8658 register map, cross-checked against
// Waveshare's Gyro_QMI8658 example. On this part SA0 low selects 0x6B (the
// board ties SA0 to GND), the reverse of most sensors.
#include "imu.h"

#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t ADDR = 0x6B;

enum : uint8_t {
  WHO_AM_I = 0x00, REVISION = 0x01,
  CTRL1 = 0x02, CTRL2 = 0x03, CTRL3 = 0x04, CTRL5 = 0x06, CTRL7 = 0x08,
  TEMP_L = 0x33,  // then AX_L..GZ_H at 0x35..0x40
  RESET = 0x60,
};

constexpr float ACC_SCALE = 4.0f / 32768.0f;    // +/-4 g
constexpr float GYR_SCALE = 512.0f / 32768.0f;  // +/-512 dps

static bool wr(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool rd(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(ADDR, len) != len) return false;
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

bool imu_begin() {
  wr(RESET, 0xB0);  // soft reset
  delay(20);
  uint8_t id[2];
  if (!rd(WHO_AM_I, id, 2)) { Serial.println("imu: no answer at 0x6b"); return false; }
  Serial.printf("imu: WHO_AM_I 0x%02x (expect 0x05), revision 0x%02x\n", id[0], id[1]);
  if (id[0] != 0x05) return false;

  wr(CTRL1, 0x40);  // register address auto-increment, little-endian
  wr(CTRL2, 0x16);  // accel: +/-4 g (aFS 001), ODR 117.5 Hz (0110)
  wr(CTRL3, 0x56);  // gyro: +/-512 dps (gFS 101), ODR 117.5 Hz (0110)
  wr(CTRL5, 0x11);  // accel and gyro low-pass filters on, mode 0
  wr(CTRL7, 0x03);  // enable accel and gyro
  delay(50);        // first samples
  return true;
}

bool imu_read(ImuSample &s) {
  uint8_t b[14];
  if (!rd(TEMP_L, b, sizeof b)) return false;
  auto i16 = [&](int i) { return (int16_t)(b[i] | b[i + 1] << 8); };
  s.temp_c = i16(0) / 256.0f;
  s.ax = i16(2) * ACC_SCALE;
  s.ay = i16(4) * ACC_SCALE;
  s.az = i16(6) * ACC_SCALE;
  s.gx = i16(8) * GYR_SCALE;
  s.gy = i16(10) * GYR_SCALE;
  s.gz = i16(12) * GYR_SCALE;
  return true;
}
