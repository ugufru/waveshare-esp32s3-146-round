// QMI8658 6-axis IMU on the shared I2C bus at 0x6B (issue 26).
#pragma once

#include <stdint.h>

struct ImuSample {
  float ax, ay, az;  // g
  float gx, gy, gz;  // degrees per second
  float temp_c;
};

// Soft-resets the chip, checks WHO_AM_I, enables accel (+/-4 g) and gyro
// (+/-512 dps) at 117.5 Hz with low-pass filters. Returns false on error.
bool imu_begin();

// Burst-reads temperature, accel and gyro. Returns false on I2C error.
bool imu_read(ImuSample &s);
