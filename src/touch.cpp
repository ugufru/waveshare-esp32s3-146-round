// Protocol ported from Espressif's esp_lcd_touch_spd2010 (esp-iot-solution,
// components/display/lcd_touch/esp_lcd_touch_spd2010, Apache-2.0), rewritten
// on Arduino Wire because the bus is shared with the expander. Each access is
// a 2-byte register write with STOP, then a separate read, as in both
// Espressif's and Waveshare's drivers.
#include "touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "expander.h"

constexpr int MAX_PACKET = 4 + TOUCH_MAX_POINTS * 6;

static volatile uint32_t irq_count = 0;
static volatile bool irq_pending = true;  // run the state machine once at start

static void IRAM_ATTR on_int() {
  irq_count++;
  irq_pending = true;
}

uint32_t touch_irq_count() { return irq_count; }

static bool reg_write(uint16_t reg, const uint8_t *data = nullptr, int len = 0) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  for (int i = 0; i < len; i++) Wire.write(data[i]);
  bool ok = Wire.endTransmission(true) == 0;
  delayMicroseconds(200);
  return ok;
}

static bool reg_read(uint16_t reg, uint8_t *buf, int len) {
  if (!reg_write(reg)) return false;
  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)len) != len) return false;
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
  delayMicroseconds(200);
  return true;
}

// Commands: register, then a 2-byte little-endian argument.
static bool cmd(uint16_t reg, uint16_t arg) {
  const uint8_t d[2] = {(uint8_t)(arg & 0xFF), (uint8_t)(arg >> 8)};
  return reg_write(reg, d, 2);
}
static bool cmd_point_mode() { return cmd(0x5000, 0x0000); }
static bool cmd_start()      { return cmd(0x4600, 0x0000); }
static bool cmd_cpu_start()  { return cmd(0x0400, 0x0001); }
static bool cmd_clear_int()  { return cmd(0x0200, 0x0001); }

bool touch_begin() {
  expander_set(EXIO_TP_RST, false);
  delay(50);
  expander_set(EXIO_TP_RST, true);
  delay(50);

  uint8_t v[18];
  if (!reg_read(0x2600, v, sizeof v)) {
    Serial.println("touch: no answer at 0x53 (display initialised first?)");
    return false;
  }
  const uint16_t dver = v[5] << 8 | v[4];
  const uint32_t pid = (uint32_t)v[9] << 24 | v[8] << 16 | v[7] << 8 | v[6];
  // IC name: two little-endian words, v[14..17] = "SPD", v[10..13] = "2010".
  char name[9] = {};
  int k = 0;
  for (int i : {17, 16, 15, 14, 13, 12, 11, 10})
    if (v[i] >= 0x20 && v[i] <= 0x7E) name[k++] = v[i];
  Serial.printf("touch: fw DVer 0x%04x PID 0x%08lx name \"%s\" raw", dver, pid, name);
  for (uint8_t b : v) Serial.printf(" %02x", b);
  Serial.println();

  pinMode(PIN_TP_INT, INPUT_PULLUP);
  attachInterrupt(PIN_TP_INT, on_int, FALLING);
  return true;
}

int touch_read(TouchPoint *pts, int max_pts) {
  // Polled on every call, as Espressif's and Waveshare's drivers are (their
  // LVGL read callback runs regardless of INT). INT is only counted.
  irq_pending = false;

  uint8_t s[4];
  if (!reg_read(0x2000, s, 4)) return 0;
  const bool pt_exist = s[0] & 0x01, gesture = s[0] & 0x02, aux = s[0] & 0x08;
  const bool in_bios = s[1] & 0x40, in_cpu = s[1] & 0x20, cpu_run = s[1] & 0x08;
  const uint16_t read_len = s[3] << 8 | s[2];
#ifdef TOUCH_DEBUG
  static uint32_t last_status = 0xFFFFFFFF;
  const uint32_t st = (uint32_t)s[0] | s[1] << 8 | s[2] << 16 | (uint32_t)s[3] << 24;
  if (st != last_status) {
    Serial.printf("touch: status %02x %02x len %u%s%s%s%s%s\n", s[0], s[1], read_len,
                  in_bios ? " BIOS" : "", in_cpu ? " CPU" : "", cpu_run ? " RUN" : "",
                  pt_exist ? " PT" : "", aux ? " AUX" : "");
    last_status = st;
  }
#endif

  if (in_bios) {  // controller just booted: start its CPU
    cmd_clear_int();
    cmd_cpu_start();
    return 0;
  }
  if (in_cpu) {   // CPU running, touch not started: point mode, start
    cmd_point_mode();
    cmd_start();
    cmd_clear_int();
    return 0;
  }
  if (cpu_run && read_len == 0) { cmd_clear_int(); return 0; }

  int n = 0;
  if (pt_exist || gesture) {
    if (read_len < 5 || read_len > MAX_PACKET) { cmd_clear_int(); return 0; }
    uint8_t p[MAX_PACKET];
    if (!reg_read(0x0003, p, read_len)) return 0;
    if (p[4] <= 0x0A && pt_exist && (read_len - 4) % 6 == 0) {
      const int count = (read_len - 4) / 6;
      for (int i = 0; i < count && n < max_pts; i++) {
        const uint8_t *r = &p[4 + i * 6];
        pts[n].id = r[0];
        pts[n].x = ((r[3] & 0xF0) << 4) | r[1];
        pts[n].y = ((r[3] & 0x0F) << 8) | r[2];
        pts[n].weight = r[4];
        n++;
      }
    }
    // Drain: read HDP status until the controller says the report is done.
    for (int guard = 0; guard < 8; guard++) {
      uint8_t h[8];
      if (!reg_read(0xFC02, h, 8)) break;
      const uint16_t next = h[2] | h[3] << 8;
      if (h[5] == 0x82) { cmd_clear_int(); break; }
      if (h[5] != 0x00 || next == 0 || next > MAX_PACKET) break;
      uint8_t rest[MAX_PACKET];
      reg_read(0x0003, rest, next);
    }
  } else if (cpu_run && aux) {
    cmd_clear_int();
  }
  return n;
}
