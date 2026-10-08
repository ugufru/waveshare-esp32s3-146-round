// LVGL 8.3.10 on the SPD2010 display and touch (issue 24).
#pragma once

// Needs display_begin() and touch_begin() done. Returns false on error.
bool lvgl_port_begin();

// Call from loop(): runs lv_timer_handler() and prints refresh stats every 5 s.
void lvgl_port_loop();
