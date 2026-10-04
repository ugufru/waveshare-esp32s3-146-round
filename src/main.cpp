// Board bring-up: USB serial hello (issue 20), shared I2C bus and TCA9554
// (issue 21), SPD2010 display test (issue 22), touch test (issue 23).
#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "display.h"
#include "expander.h"
#include "touch.h"

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

// --- Display test patterns ---------------------------------------------------

constexpr uint16_t RED = rgb565(255, 0, 0), GREEN = rgb565(0, 255, 0),
                   BLUE = rgb565(0, 0, 255), WHITE = rgb565(255, 255, 255),
                   YELLOW = rgb565(255, 255, 0), GREY = rgb565(40, 40, 40),
                   BLACK = 0;

static bool touch_ok = false;
static uint16_t fill_color;
static uint16_t solid(int, int) { return fill_color; }

// Orientation pattern, in panel memory coordinates (row 0 = "top"):
// a coloured wedge per edge (top red, right green, bottom blue, left yellow),
// a white ring just inside the round visible area, and a white crosshair.
static uint16_t orientation(int x, int y) {
  const int dx = 2 * x - (LCD_W - 1), dy = 2 * y - (LCD_H - 1);  // doubled, centred
  const int r2 = dx * dx + dy * dy;                               // (2r)^2
  const int R = LCD_W;                                            // 2 x radius 206
  if (r2 > R * R) return BLACK;                                   // outside the circle
  if (r2 >= (R - 12) * (R - 12)) return WHITE;                    // ring, 6 px
  if (abs(dx) <= 1 || abs(dy) <= 1) return WHITE;                 // crosshair
  if (r2 >= (R / 2) * (R / 2)) {                                  // outer half: wedges
    if (abs(dx) < abs(dy) / 3) return dy < 0 ? RED : BLUE;
    if (abs(dy) < abs(dx) / 3) return dx > 0 ? GREEN : YELLOW;
  }
  return GREY;
}

static void display_test() {
  if (!display_begin()) { Serial.println("display: FAILED"); return; }
  Serial.println("display: init ok");

  const struct { uint16_t c; const char *name; } fills[] = {
      {RED, "red"}, {GREEN, "green"}, {BLUE, "blue"}, {WHITE, "white"}, {BLACK, "black"}};
  for (auto &f : fills) {
    fill_color = f.c;
    uint32_t us = display_fill(solid);
    if (&f == &fills[0]) display_backlight(60);  // on once there is a defined image
    Serial.printf("display: fill %-5s %lu us\n", f.name, us);
    delay(f.c == BLACK ? 100 : 1000);
  }

  // Refresh rate: back-to-back full-screen fills.
  fill_color = BLACK;
  const int N = 20;
  uint32_t t0 = micros();
  for (int i = 0; i < N; i++) { fill_color = (i & 1) ? GREY : BLACK; display_fill(solid); }
  uint32_t per = (micros() - t0) / N;
  Serial.printf("display: full-screen fill %lu us = %.1f fps (40 MHz QSPI, %d-row strips)\n",
                per, 1e6f / per, 40);

  uint32_t us = display_fill(orientation);
  Serial.printf("display: orientation pattern %lu us\n", us);
}

void setup() {
  Serial.begin(115200);
  // Give the host up to 3 s to open the port so the banner is not lost.
  for (uint32_t t0 = millis(); !Serial && millis() - t0 < 3000;) delay(10);
  Serial.println("\nwaveshare-esp32s3-146-round bring-up");
  print_info();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  i2c_scan("boot");
  if (!expander_begin()) {
    Serial.println("tca9554 NOT FOUND in 0x20..0x27");
    return;
  }
  expander_dump("init");

  // Touch reset pulse (touch itself is issue 23).
  expander_set(EXIO_TP_RST, false);
  delay(20);
  expander_set(EXIO_TP_RST, true);
  delay(200);

  display_test();
  i2c_scan("after display init");  // SPD2010 touch (0x53) appears only now
  touch_ok = touch_begin();
  if (touch_ok) Serial.println("touch: ready, draw on the screen");
}

// Touch test: a dot under each finger, coordinates on serial. The canvas is
// redrawn after 5 s without touch.
static void dot(int cx, int cy, uint16_t c) {
  constexpr int D = 8;
  static uint16_t px[D * D];
  for (auto &p : px) p = c;
  // Align to 4 px in x/y in case the controller wants aligned windows.
  int x0 = constrain((cx - D / 2) & ~3, 0, LCD_W - D);
  int y0 = constrain((cy - D / 2) & ~3, 0, LCD_H - D);
  display_draw(x0, y0, x0 + D, y0 + D, px);
}

void loop() {
  static uint32_t n = 0, next_beat = 0, last_touch = 0;
  static bool dirty = false;
  static uint16_t last_x[TOUCH_MAX_POINTS], last_y[TOUCH_MAX_POINTS];

  if (touch_ok) {
    TouchPoint pts[TOUCH_MAX_POINTS];
    const int k = touch_read(pts, TOUCH_MAX_POINTS);
    for (int i = 0; i < k; i++) {
      const uint16_t colors[] = {WHITE, RED, GREEN, BLUE, YELLOW};
      if (pts[i].weight == 0) {
        Serial.printf("touch: id %u up   x %3u y %3u\n", pts[i].id, pts[i].x, pts[i].y);
        continue;
      }
      if (pts[i].id < TOUCH_MAX_POINTS && (pts[i].x != last_x[pts[i].id] || pts[i].y != last_y[pts[i].id])) {
        Serial.printf("touch: id %u x %3u y %3u w %u (%d pt)\n", pts[i].id, pts[i].x, pts[i].y,
                      pts[i].weight, k);
        last_x[pts[i].id] = pts[i].x;
        last_y[pts[i].id] = pts[i].y;
      }
      dot(pts[i].x, pts[i].y, colors[pts[i].id % 5]);
      last_touch = millis();
      dirty = true;
    }
    if (dirty && millis() - last_touch > 5000) {
      display_fill(orientation);
      dirty = false;
    }
  }

  if ((int32_t)(millis() - next_beat) >= 0) {
    next_beat = millis() + 10000;
    Serial.printf("[%lu] up %lu ms, heap free %lu, touch irqs %lu\n", n++, millis(),
                  ESP.getFreeHeap(), touch_irq_count());
  }
  delay(10);
}
