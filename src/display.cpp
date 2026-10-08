#include "display.h"

#include <Arduino.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/semphr.h>

#include "board_pins.h"
#include "esp_lcd_spd2010.h"
#include "expander.h"

// Values from Waveshare's Arduino 3.1.1 example (Display_SPD2010.h), which is
// tested on this board: SPI mode 3, 40 MHz (upstream's default is 20 MHz),
// 32-bit commands, 8-bit params.
constexpr spi_host_device_t LCD_HOST = SPI2_HOST;
constexpr int LCD_PCLK_HZ = 40 * 1000 * 1000;
constexpr int BL_PWM_HZ = 20000;
constexpr int BL_PWM_BITS = 10;

// Full-width strips, double-buffered in internal DMA-capable RAM.
constexpr int STRIP_ROWS = 40;
constexpr size_t STRIP_BYTES = LCD_W * STRIP_ROWS * sizeof(uint16_t);

static esp_lcd_panel_handle_t panel = nullptr;
static uint16_t *strip[2] = {nullptr, nullptr};
// Counts strip buffers not owned by an in-flight transfer.
static SemaphoreHandle_t free_bufs = nullptr;

// One-shot completion hook for display_draw_async(); when unset, a finished
// transfer releases a strip buffer instead.
static void (*volatile async_done)(void *) = nullptr;
static void *volatile async_ctx = nullptr;

static bool IRAM_ATTR on_color_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *,
                                    void *) {
  if (async_done) {
    void (*cb)(void *) = async_done;
    async_done = nullptr;
    cb(async_ctx);
    return false;
  }
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(free_bufs, &woken);
  return woken == pdTRUE;
}

static inline uint16_t bus_order(uint16_t c) { return (c >> 8) | (c << 8); }

bool display_begin() {
  // Backlight off until the panel shows something defined.
  ledcAttach(PIN_LCD_BL, BL_PWM_HZ, BL_PWM_BITS);
  ledcWrite(PIN_LCD_BL, 0);

  // Hardware reset through the expander (Waveshare: low 50 ms, high 50 ms).
  expander_set(EXIO_LCD_RST, false);
  delay(50);
  expander_set(EXIO_LCD_RST, true);
  delay(50);

  free_bufs = xSemaphoreCreateCounting(2, 2);

  // The driver's SPD2010_PANEL_*_CONFIG macros use C designated-initialiser
  // order that C++ rejects, so the same values are set field by field.
  spi_bus_config_t bus = {};
  bus.sclk_io_num = PIN_LCD_SCK;
  bus.data0_io_num = PIN_LCD_SDA0;
  bus.data1_io_num = PIN_LCD_SDA1;
  bus.data2_io_num = PIN_LCD_SDA2;
  bus.data3_io_num = PIN_LCD_SDA3;
  bus.data4_io_num = bus.data5_io_num = bus.data6_io_num = bus.data7_io_num = -1;
  bus.max_transfer_sz = STRIP_BYTES;
  esp_err_t err = spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO);
  if (err != ESP_OK) { Serial.printf("display: spi bus %s\n", esp_err_to_name(err)); return false; }

  esp_lcd_panel_io_spi_config_t io_cfg = {};
  io_cfg.cs_gpio_num = PIN_LCD_CS;
  io_cfg.dc_gpio_num = -1;
  io_cfg.spi_mode = 3;
  io_cfg.pclk_hz = LCD_PCLK_HZ;
  io_cfg.trans_queue_depth = 10;
  io_cfg.on_color_trans_done = on_color_done;
  io_cfg.lcd_cmd_bits = 32;
  io_cfg.lcd_param_bits = 8;
  io_cfg.flags.quad_mode = 1;
  esp_lcd_panel_io_handle_t io = nullptr;
  err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io);
  if (err != ESP_OK) { Serial.printf("display: panel io %s\n", esp_err_to_name(err)); return false; }

  spd2010_vendor_config_t vendor = {};
  vendor.flags.use_qspi_interface = 1;
  esp_lcd_panel_dev_config_t dev = {};
  dev.reset_gpio_num = -1;  // reset is on the expander, done above
  dev.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
  dev.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
  dev.bits_per_pixel = 16;
  dev.vendor_config = &vendor;
  err = esp_lcd_new_panel_spd2010(io, &dev, &panel);
  if (err != ESP_OK) { Serial.printf("display: new panel %s\n", esp_err_to_name(err)); return false; }

  esp_lcd_panel_reset(panel);  // software reset (no reset GPIO)
  err = esp_lcd_panel_init(panel);
  if (err != ESP_OK) { Serial.printf("display: init %s\n", esp_err_to_name(err)); return false; }
  esp_lcd_panel_disp_on_off(panel, true);
  return true;
}

void display_backlight(uint8_t percent) {
  if (percent > 100) percent = 100;
  ledcWrite(PIN_LCD_BL, (uint32_t)percent * ((1 << BL_PWM_BITS) - 1) / 100);
}

// Strip buffers for the blocking calls, allocated on first use so that a
// program drawing only through display_draw_async (LVGL) does not pay 66 KB
// of internal RAM for them.
static bool strips_ready() {
  for (auto &b : strip) {
    if (!b) b = (uint16_t *)heap_caps_malloc(STRIP_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!b) { Serial.println("display: strip alloc failed"); return false; }
  }
  return true;
}

// Wait until both strip buffers are idle (no transfer in flight).
static void wait_idle() {
  xSemaphoreTake(free_bufs, portMAX_DELAY);
  xSemaphoreTake(free_bufs, portMAX_DELAY);
  xSemaphoreGive(free_bufs);
  xSemaphoreGive(free_bufs);
}

void display_draw(int x0, int y0, int x1, int y1, const uint16_t *pixels) {
  if (!strips_ready()) return;
  const int w = x1 - x0;
  const int rows_per = STRIP_ROWS * LCD_W / w;  // fill a strip buffer
  int b = 0;
  for (int y = y0; y < y1; y += rows_per, b ^= 1) {
    const int ye = min(y + rows_per, y1);
    xSemaphoreTake(free_bufs, portMAX_DELAY);
    const size_t n = (size_t)w * (ye - y);
    for (size_t i = 0; i < n; i++) strip[b][i] = bus_order(pixels[(size_t)(y - y0) * w + i]);
    esp_lcd_panel_draw_bitmap(panel, x0, y, x1, ye, strip[b]);
  }
  wait_idle();
}

uint32_t display_fill(uint16_t (*pixel)(int x, int y)) {
  if (!strips_ready()) return 0;
  const uint32_t t0 = micros();
  int b = 0;
  for (int y = 0; y < LCD_H; y += STRIP_ROWS, b ^= 1) {
    const int ye = min(y + STRIP_ROWS, LCD_H);
    xSemaphoreTake(free_bufs, portMAX_DELAY);
    uint16_t *p = strip[b];
    for (int yy = y; yy < ye; yy++)
      for (int x = 0; x < LCD_W; x++) *p++ = bus_order(pixel(x, yy));
    esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_W, ye, strip[b]);
  }
  wait_idle();
  return micros() - t0;
}

void display_draw_async(int x0, int y0, int x1, int y1, const void *bus_pixels,
                        void (*done)(void *), void *ctx) {
  async_ctx = ctx;
  async_done = done;
  esp_lcd_panel_draw_bitmap(panel, x0, y0, x1, y1, bus_pixels);
}
