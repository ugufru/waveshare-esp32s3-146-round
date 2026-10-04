// SPD2010 412x412 panel over QSPI with backlight (issue 22).
#pragma once

#include <stdint.h>

constexpr int LCD_W = 412;
constexpr int LCD_H = 412;

// Needs the expander up (LCD reset is EXIO_LCD_RST). Returns false on error.
bool display_begin();

// 0..100 %.
void display_backlight(uint8_t percent);

// Blocking draw of an RGB565 rectangle, x1/y1 exclusive. Pixels are native
// RGB565; byte swapping for the bus is done here.
void display_draw(int x0, int y0, int x1, int y1, const uint16_t *pixels);

// Fill the whole screen, strip by strip, with a per-pixel colour function.
// Returns the elapsed time in microseconds.
uint32_t display_fill(uint16_t (*pixel)(int x, int y));

inline constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
