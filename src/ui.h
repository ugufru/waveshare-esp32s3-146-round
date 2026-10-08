// Demo UI for the round screen (issue 24).
#pragma once

void ui_build();

// Battery readout in the ring's gap at 6 o'clock.
void ui_set_battery(float volts, int percent);

// Full-screen message over everything (e.g. before power-off), drawn at once.
void ui_show_message(const char *text);

// Current brightness set on the ring, 5..100.
int ui_brightness();

// Bubble dot: uphill tilt in g along the screen's x (right) and y (down)
// axes; the dot moves that way, towards the raised edge, like a spirit level.
void ui_set_tilt(float x, float y);
