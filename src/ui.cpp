// A round-screen demo that exercises display, touch and LVGL together:
// a brightness arc around the rim (dragging it drives the real backlight),
// the value in the centre, a tap counter button, local time and date at
// 12 o'clock (from the RTC, issue 26) and a spirit-level bubble driven by
// the IMU.
// Panel row 0 is 12 o'clock in the pocket-watch case (issue 22), so the
// arc's gap sits at 6 o'clock over the USB-C port.
#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>

#include "display.h"
#include "rtc.h"

static lv_obj_t *value_label;
static lv_obj_t *tap_label;
static lv_obj_t *time_label;
static lv_obj_t *bat_label;
static lv_obj_t *date_label;
static lv_obj_t *bubble;
static uint32_t taps = 0;
static int brightness = 60;

static void on_arc(lv_event_t *e) {
  const int v = lv_arc_get_value(lv_event_get_target(e));
  brightness = v;
  display_backlight(v);
  lv_label_set_text_fmt(value_label, "%d%%", v);
}

static void on_tap(lv_event_t *) {
  lv_label_set_text_fmt(tap_label, "taps: %lu", ++taps);
}

static void on_tick(lv_timer_t *) {
  if (!rtc_valid()) {
    lv_label_set_text(time_label, "--:--:--");
    lv_label_set_text(date_label, "time not set");
    return;
  }
  static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const time_t local = time(nullptr) + rtc_offset_min() * 60;
  tm t;
  gmtime_r(&local, &t);
  lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  lv_label_set_text_fmt(date_label, "%s %d %s", wday[t.tm_wday], t.tm_mday, mon[t.tm_mon]);
}

void ui_build() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

  lv_obj_t *arc = lv_arc_create(scr);
  lv_obj_set_size(arc, 396, 396);
  lv_obj_center(arc);
  lv_arc_set_bg_angles(arc, 135, 45);  // 270 degrees, gap at 6 o'clock
  lv_arc_set_range(arc, 5, 100);
  lv_arc_set_value(arc, 60);
  lv_obj_set_style_arc_width(arc, 22, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 22, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0x2a3038), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0xf0a020), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(arc, lv_color_hex(0xffffff), LV_PART_KNOB);
  lv_obj_set_style_pad_all(arc, 6, LV_PART_KNOB);
  lv_obj_add_event_cb(arc, on_arc, LV_EVENT_VALUE_CHANGED, nullptr);

  value_label = lv_label_create(scr);
  lv_obj_set_style_text_font(value_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(value_label, lv_color_hex(0xffffff), 0);
  lv_label_set_text(value_label, "60%");
  lv_obj_align(value_label, LV_ALIGN_CENTER, 0, -30);

  lv_obj_t *caption = lv_label_create(scr);
  lv_obj_set_style_text_color(caption, lv_color_hex(0x8a94a0), 0);
  lv_label_set_text(caption, "brightness: drag the ring");
  lv_obj_align(caption, LV_ALIGN_CENTER, 0, 10);

  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_set_size(btn, 150, 56);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 75);
  lv_obj_add_event_cb(btn, on_tap, LV_EVENT_CLICKED, nullptr);
  tap_label = lv_label_create(btn);
  lv_obj_set_style_text_font(tap_label, &lv_font_montserrat_28, 0);
  lv_label_set_text(tap_label, "taps: 0");
  lv_obj_center(tap_label);

  time_label = lv_label_create(scr);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), 0);
  lv_label_set_text(time_label, "--:--:--");
  lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 62);

  date_label = lv_label_create(scr);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0x8a94a0), 0);
  lv_label_set_text(date_label, "");
  lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 98);
  lv_timer_create(on_tick, 250, nullptr);

  bubble = lv_obj_create(scr);
  lv_obj_set_size(bubble, 18, 18);
  lv_obj_set_style_radius(bubble, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(bubble, lv_color_hex(0x40c0ff), 0);
  lv_obj_set_style_border_width(bubble, 0, 0);
  lv_obj_clear_flag(bubble, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(bubble, LV_ALIGN_CENTER, 0, 0);
  lv_obj_move_background(bubble);

  bat_label = lv_label_create(scr);
  lv_obj_set_style_text_color(bat_label, lv_color_hex(0x8a94a0), 0);
  lv_label_set_text(bat_label, "bat --");
  lv_obj_align(bat_label, LV_ALIGN_BOTTOM_MID, 0, -40);
}

void ui_set_battery(float volts, int percent) {
  lv_label_set_text_fmt(bat_label, "bat %d.%02d V  %d%%", (int)volts,
                        (int)(volts * 100 + 0.5f) % 100, percent);
}

void ui_show_message(const char *text) {
  lv_obj_t *cover = lv_obj_create(lv_layer_top());
  lv_obj_set_size(cover, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(cover, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_width(cover, 0, 0);
  lv_obj_t *l = lv_label_create(cover);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  lv_refr_now(nullptr);
}

int ui_brightness() { return brightness; }

void ui_set_tilt(float x, float y) {
  // 1 g of tilt moves the dot 150 px; clamp inside the ring.
  float px = x * 150, py = y * 150;
  const float r = sqrtf(px * px + py * py), rmax = 160;
  if (r > rmax) { px *= rmax / r; py *= rmax / r; }
  lv_obj_align(bubble, LV_ALIGN_CENTER, (lv_coord_t)px, (lv_coord_t)py);
}
