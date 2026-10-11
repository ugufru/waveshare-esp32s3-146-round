// Pin map for the Waveshare ESP32-S3-Touch-LCD-1.46 (issue 19).
// Source: Waveshare's schematic (link in docs/pins.md), page 1 GPIO
// table and the IO Extension block. Notes and gotchas: docs/pins.md.
#pragma once

#include <stdint.h>

// --- LCD: SPD2010, 412x412, QSPI ---------------------------------------------
constexpr int PIN_LCD_CS   = 21;
constexpr int PIN_LCD_SCK  = 40;
constexpr int PIN_LCD_SDA0 = 46;  // strapping pin
constexpr int PIN_LCD_SDA1 = 45;  // strapping pin (VDD_SPI voltage)
constexpr int PIN_LCD_SDA2 = 42;
constexpr int PIN_LCD_SDA3 = 41;
constexpr int PIN_LCD_TE   = 18;
constexpr int PIN_LCD_BL   = 5;   // BL_PWM, drives the MP1605 boost via Q3
// LCD reset is on the expander: EXIO_LCD_RST.

// --- Touch: SPD2010 on the shared I2C bus ------------------------------------
constexpr int PIN_TP_INT = 4;
// Touch reset is on the expander: EXIO_TP_RST.

// --- Shared I2C bus: touch, QMI8658, PCF85063, TCA9554 -----------------------
constexpr int PIN_I2C_SCL = 10;
constexpr int PIN_I2C_SDA = 11;

// --- RTC: PCF85063 -----------------------------------------------------------
constexpr int PIN_RTC_INT = 9;

// --- microSD: TF-07F, SPI mode -----------------------------------------------
constexpr int PIN_SD_SCLK = 14;
constexpr int PIN_SD_MISO = 16;  // D0
constexpr int PIN_SD_MOSI = 17;  // CMD
// SD chip select is on the expander: EXIO_SD_CS.

// --- Audio out: PCM5101 DAC -> NS8002 amp -> speaker -------------------------
constexpr int PIN_I2S_BCK  = 48;
constexpr int PIN_I2S_DIN  = 47;
constexpr int PIN_I2S_LRCK = 38;

// --- Microphone: MSM261S4030, I2S --------------------------------------------
constexpr int PIN_MIC_WS  = 2;
constexpr int PIN_MIC_SCK = 15;
constexpr int PIN_MIC_SD  = 39;

// --- Power -------------------------------------------------------------------
constexpr int PIN_KEY_BAT     = 6;  // PWR key sense
constexpr int PIN_BAT_CONTROL = 7;  // power-hold latch: drive high early on battery
constexpr int PIN_BAT_ADC     = 8;  // 200k/100k divider: Vbat = 3 x Vadc

// --- Buttons, USB, header ----------------------------------------------------
constexpr int PIN_BOOT   = 0;   // BOOT key, strapping pin
constexpr int PIN_USB_DN = 19;  // native USB, do not use
constexpr int PIN_USB_DP = 20;
// Free on the 2x10 1.27 mm header J9: GPIO1, 3, 12, 13, UART0 TXD 43 / RXD 44.
// GPIO33..37 are used by the octal PSRAM ("Internal occupancy"): never touch.

// --- TCA9554 I/O expander ----------------------------------------------------
// A1 and A2 are tied to GND; A0 is a 0R/NC option (R50). The address actually
// fitted is recorded in docs/pins.md from the I2C scan (issue 21).
// INT (pin 13) reaches GPIO0 only through R42, which is not fitted: poll it.
//
// The schematic labels the outputs Extend_IO1..8 on chip ports P0..P7, so
// EXIOn is bit (n - 1). These constants are the port bit numbers.
constexpr uint8_t EXIO_TP_RST   = 0;  // Extend_IO1
constexpr uint8_t EXIO_LCD_RST  = 1;  // Extend_IO2
constexpr uint8_t EXIO_SD_CS    = 2;  // Extend_IO3
constexpr uint8_t EXIO_IMU_INT2 = 3;  // Extend_IO4 (input)
constexpr uint8_t EXIO_IMU_INT1 = 4;  // Extend_IO5 (input)
// Extend_IO6..8 (P5..P7) are spare, pulled up to 3V3 by 10k.
