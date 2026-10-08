// Board bring-up and demo. Start-up order matters (docs/pins.md):
// I2C -> TCA9554 expander (holds the LCD and touch resets) -> display ->
// touch (the SPD2010 touch side answers only once the display is up) -> LVGL.
// The raw display and touch test patterns of issues 22 and 23 are in git
// history (commit 6139609).
#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "display.h"
#include "expander.h"
#include "lvgl_port.h"
#include "touch.h"
#include "ui.h"

static void print_info() {
  Serial.printf("chip %s rev %d, %d cores @ %lu MHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("flash %lu bytes @ %lu Hz\n", ESP.getFlashChipSize(),
                ESP.getFlashChipSpeed());
  Serial.printf("psram %lu bytes (%s)\n", ESP.getPsramSize(),
                psramFound() ? "found" : "NOT FOUND");
}

static void i2c_scan(const char *when) {
  Serial.printf("i2c scan (%s):", when);
  int n = 0;
  for (uint8_t a = 0x08; a < 0x78; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02x", a); n++; }
  }
  Serial.printf("  [%d device%s]\n", n, n == 1 ? "" : "s");
}

static bool ok = false;

void setup() {
  Serial.begin(115200);
  // Give the host up to 3 s to open the port so the banner is not lost.
  for (uint32_t t0 = millis(); !Serial && millis() - t0 < 3000;) delay(10);
  Serial.println("\nwaveshare-esp32s3-146-round");
  print_info();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  if (!expander_begin()) { Serial.println("tca9554 NOT FOUND in 0x20..0x27"); return; }
  if (!display_begin()) { Serial.println("display: FAILED"); return; }
  if (!touch_begin()) return;
  i2c_scan("after init");
  if (!lvgl_port_begin()) return;

  ui_build();
  lvgl_port_loop();        // first frame, then light it
  display_backlight(60);
  ok = true;
}

void loop() {
  if (!ok) { delay(1000); return; }
  lvgl_port_loop();
  delay(2);
}
