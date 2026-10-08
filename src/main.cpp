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
#include "power.h"
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
  power_begin();  // first: on battery the board stays on only once this latches
  Serial.begin(115200);
  // Give the host up to 3 s to open the port so the banner is not lost.
  for (uint32_t t0 = millis(); !Serial && millis() - t0 < 3000;) delay(10);
  Serial.println("\nwaveshare-esp32s3-146-round");
  print_info();
  static const char *src[] = {"power key", "usb", "wake from sleep"};
  Serial.printf("power: boot source %s, battery %.2f V\n",
                src[(int)power_boot_source()], power_battery_volts());

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

  static bool screen_on = true;
  switch (power_key_poll()) {
    case KeyEvent::Short:  // toggle the screen
      screen_on = !screen_on;
      display_backlight(screen_on ? ui_brightness() : 0);
      Serial.printf("power: key short, screen %s\n", screen_on ? "on" : "off");
      break;
    case KeyEvent::LongHeld:
      Serial.println("power: key long, powering off on release");
      display_backlight(ui_brightness());
      ui_show_message("Release PWR\nto power off");
      power_off();
    case KeyEvent::None:
      break;
  }

  static uint32_t next_bat = 0;
  if ((int32_t)(millis() - next_bat) >= 0) {
    next_bat = millis() + 5000;
    const float v = power_battery_volts();
    ui_set_battery(v, power_battery_percent(v));
    Serial.printf("power: battery %.3f V (%d%%), key %s\n", v, power_battery_percent(v),
                  digitalRead(PIN_KEY_BAT) == LOW ? "down" : "up");
  }
  delay(2);
}
