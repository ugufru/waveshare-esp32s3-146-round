// Board bring-up and demo. Start-up order matters (docs/pins.md):
// I2C -> TCA9554 expander (holds the LCD and touch resets) -> display ->
// touch (the SPD2010 touch side answers only once the display is up) -> RTC,
// IMU -> Wi-Fi/NTP -> LVGL.
// The raw display and touch test patterns of issues 22 and 23 are in git
// history (commit 6139609).
#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "display.h"
#include "expander.h"
#include "imu.h"
#include "lvgl_port.h"
#include "net.h"
#include "power.h"
#include "rtc.h"
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
static bool imu_ok = false;

// Serial commands, one per line:
//   t <unix_utc> <offset_min>    set the RTC (offset a multiple of 15)
//   tz <offset_min>              change only the local offset
//   wifi "<ssid>" "<password>"   store Wi-Fi credentials in NVS and connect
//   wifi                         Wi-Fi status;  wifi clear   forget them
//   ntp                          SNTP diagnostics
// Wi-Fi lines are never echoed back (they carry the password).
// From a Mac terminal (port name varies, see docs/BUILD.md):
//   printf 't %d -240\n' $(date +%s) > /dev/cu.usbmodem101

// Splits a "quoted" or bare word off *p; returns false at end of line.
static bool next_word(char *&p, char *out, size_t cap) {
  while (*p == ' ') p++;
  if (!*p) return false;
  size_t n = 0;
  if (*p == '"') {
    for (p++; *p && *p != '"'; p++) if (n < cap - 1) out[n++] = *p;
    if (*p == '"') p++;
  } else {
    for (; *p && *p != ' '; p++) if (n < cap - 1) out[n++] = *p;
  }
  out[n] = 0;
  return true;
}

static void run_command(char *line) {
  long long utc;
  int off;
  if (strncmp(line, "wifi", 4) == 0 && (line[4] == 0 || line[4] == ' ')) {
    char *p = line + 4;
    char ssid[33], pass[65];
    if (!next_word(p, ssid, sizeof ssid)) net_print_status();
    else if (strcmp(ssid, "clear") == 0) net_clear_credentials();
    else if (strcmp(ssid, "reconnect") == 0) net_reconnect(30000);
    else if (next_word(p, pass, sizeof pass)) net_set_credentials(ssid, pass);
    else Serial.println("? usage: wifi \"<ssid>\" \"<password>\"  (line not echoed)");
    memset(line, 0, strlen(line));
  } else if (sscanf(line, "t %lld %d", &utc, &off) == 2 && off % 15 == 0 && off >= -720 && off <= 840) {
    const bool w = rtc_set((time_t)utc, off);
    Serial.printf("rtc: set %lld UTC offset %+d min: %s\n", utc, off, w ? "ok" : "I2C error");
  } else if (sscanf(line, "tz %d", &off) == 1 && off % 15 == 0 && off >= -720 && off <= 840) {
    const bool w = rtc_set(time(nullptr), off);
    Serial.printf("rtc: offset %+d min: %s\n", off, w ? "ok" : "I2C error");
  } else if (strcmp(line, "ntp") == 0) {
    net_print_ntp();
  } else if (strcmp(line, "ntptest") == 0) {
    net_ntp_test();
  } else if (line[0]) {
    Serial.printf("? '%s' (commands: t, tz, wifi, ntp)\n", line);
  }
}

static void serial_commands() {
  static char line[128];
  static size_t len = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c != '\n' && c != '\r') {
      if (len < sizeof line - 1) line[len++] = c;
      continue;
    }
    line[len] = 0;
    len = 0;
    run_command(line);
  }
}

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
  rtc_begin();
  imu_ok = imu_begin();
  net_begin();
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

  serial_commands();
  net_loop();

  static uint32_t next_imu = 0, next_imu_log = 0;
  if (imu_ok && (int32_t)(millis() - next_imu) >= 0) {
    next_imu = millis() + 33;
    ImuSample m;
    if (imu_read(m)) {
      // IMU axes (measured, issue 26): +x towards 12 o'clock, +y towards
      // 3 o'clock, +z out of the back. A reading is positive when that axis
      // points up, so the uphill direction on screen (x right, y down) is
      // (+ay, -ax): the dot behaves like a spirit-level bubble.
      ui_set_tilt(m.ay, -m.ax);
      if ((int32_t)(millis() - next_imu_log) >= 0) {
        next_imu_log = millis() + 1000;
        Serial.printf("imu: a %+.3f %+.3f %+.3f g |a| %.3f  g %+7.2f %+7.2f %+7.2f dps  %.1f C\n",
                      m.ax, m.ay, m.az, sqrtf(m.ax * m.ax + m.ay * m.ay + m.az * m.az),
                      m.gx, m.gy, m.gz, m.temp_c);
      }
    }
  }

  static uint32_t next_bat = 0;
  if ((int32_t)(millis() - next_bat) >= 0) {
    next_bat = millis() + 5000;
    const float v = power_battery_volts();
    ui_set_battery(v, power_battery_percent(v));
    ui_set_net(net_ui_status());
    Serial.printf("power: battery %.3f V (%d%%), key %s\n", v, power_battery_percent(v),
                  digitalRead(PIN_KEY_BAT) == LOW ? "down" : "up");
  }
  delay(2);
}
