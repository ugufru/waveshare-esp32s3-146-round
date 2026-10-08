// A round-screen demo that exercises display, touch and LVGL together:
// a brightness arc around the rim (dragging it drives the real backlight),
// the value in the centre, a tap counter button, and uptime at 12 o'clock.
// Panel row 0 is 12 o'clock in the pocket-watch case (issue 22), so the
// arc's gap sits at 6 o'clock over the USB-C port.
#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>

#include "display.h"

static lv_obj_t *value_label;
static lv_obj_t *tap_label;
static lv_obj_t *time_label;
static uint32_t taps = 0;

static void on_arc(lv_event_t *e) {
  const int v = lv_arc_get_value(lv_event_get_target(e));
  display_backlight(v);
  lv_label_set_text_fmt(value_label, "%d%%", v);
}

static void on_tap(lv_event_t *) {
  lv_label_set_text_fmt(tap_label, "taps: %lu", ++taps);
}

static void on_tick(lv_timer_t *) {
  const uint32_t s = millis() / 1000;
  lv_label_set_text_fmt(time_label, "%02lu:%02lu", s / 60, s % 60);
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
  lv_label_set_text(time_label, "00:00");
  lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 70);
  lv_timer_create(on_tick, 1000, nullptr);
}
