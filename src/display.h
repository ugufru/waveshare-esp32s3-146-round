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

// Start a DMA draw of pixels already in bus byte order (big-endian RGB565,
// e.g. LVGL with LV_COLOR_16_SWAP) and return at once; done(ctx) runs from
// the SPI interrupt when the transfer ends. The buffer must stay valid and
// DMA-capable until then. x1/y1 exclusive. Do not mix with the blocking
// calls while a transfer is in flight. The SPD2010 wants x0 and x1 multiples
// of 4 (Waveshare's LVGL rounder).
void display_draw_async(int x0, int y0, int x1, int y1, const void *bus_pixels,
                        void (*done)(void *), void *ctx);

// Fill the whole screen, strip by strip, with a per-pixel colour function.
// Returns the elapsed time in microseconds.
uint32_t display_fill(uint16_t (*pixel)(int x, int y));

inline constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
