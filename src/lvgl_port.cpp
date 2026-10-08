#include "lvgl_port.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "display.h"
#include "touch.h"

// Two partial draw buffers in internal DMA-capable RAM, so the SPI DMA reads
// them directly while LVGL renders into the other one.
constexpr int BUF_ROWS = 40;
constexpr size_t BUF_PX = (size_t)LCD_W * BUF_ROWS;

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// Refresh statistics from the monitor callback.
static volatile uint32_t stat_frames = 0, stat_ms = 0, stat_px = 0;

static void flush_done(void *drv) { lv_disp_flush_ready((lv_disp_drv_t *)drv); }

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *a, lv_color_t *px) {
  // LV_COLOR_16_SWAP: px is already big-endian RGB565 as the bus wants it.
  display_draw_async(a->x1, a->y1, a->x2 + 1, a->y2 + 1, px, flush_done, drv);
}

// The SPD2010 needs x0 a multiple of 4 and the width a multiple of 4
// (Waveshare's LVGL port does the same).
static void rounder_cb(lv_disp_drv_t *, lv_area_t *a) {
  a->x1 &= ~3;
  a->x2 |= 3;
}

static void monitor_cb(lv_disp_drv_t *, uint32_t time_ms, uint32_t px) {
  stat_frames++;
  stat_ms += time_ms;
  stat_px += px;
}

static void touch_cb(lv_indev_drv_t *, lv_indev_data_t *data) {
  static lv_coord_t x = 0, y = 0;
  TouchPoint pts[TOUCH_MAX_POINTS];
  const int n = touch_read(pts, TOUCH_MAX_POINTS);
  if (n > 0 && pts[0].weight > 0) {
    x = pts[0].x;
    y = pts[0].y;
  }
  data->point.x = x;
  data->point.y = y;
  data->state = touch_down() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
#ifdef TOUCH_DEBUG
  static lv_indev_state_t last = LV_INDEV_STATE_REL;
  if (data->state != last || n > 0) {
    Serial.printf("%7lu indev %s n %d w %d x %d y %d\n", millis(),
                  data->state == LV_INDEV_STATE_PR ? "PR " : "REL", n, n ? pts[0].weight : -1, x, y);
    last = data->state;
  }
#endif
}

bool lvgl_port_begin() {
  lv_init();

  auto *b1 = (lv_color_t *)heap_caps_malloc(BUF_PX * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  auto *b2 = (lv_color_t *)heap_caps_malloc(BUF_PX * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!b1 || !b2) { Serial.println("lvgl: draw buffer alloc failed"); return false; }
  lv_disp_draw_buf_init(&draw_buf, b1, b2, BUF_PX);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_W;
  disp_drv.ver_res = LCD_H;
  disp_drv.flush_cb = flush_cb;
  disp_drv.rounder_cb = rounder_cb;
  disp_drv.monitor_cb = monitor_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touch_cb;
  lv_indev_drv_register(&indev_drv);

  Serial.printf("lvgl: %d.%d.%d, 2 x %u B draw buffers (%d rows) in internal DMA RAM\n",
                LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
                (unsigned)(BUF_PX * sizeof(lv_color_t)), BUF_ROWS);
  return true;
}

void lvgl_port_loop() {
  lv_timer_handler();

  static uint32_t next = 0;
  if ((int32_t)(millis() - next) >= 0) {
    next = millis() + 5000;
    const uint32_t f = stat_frames, ms = stat_ms, px = stat_px;
    stat_frames = stat_ms = stat_px = 0;
    Serial.printf("lvgl: %lu refreshes in 5 s (%.1f/s), avg %lu ms and %lu px each; heap %lu, psram %lu\n",
                  f, f / 5.0f, f ? ms / f : 0, f ? px / f : 0, ESP.getFreeHeap(), ESP.getFreePsram());
  }
}
