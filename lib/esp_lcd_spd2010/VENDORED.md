# Vendored: esp_lcd_spd2010

Source: github.com/espressif/esp-iot-solution,
`components/display/lcd/esp_lcd_spd2010`, version 2.0.0~1, at commit
88152bff24d5fbc05ff2ffed3d5208c247711ded (2026-09-30). Apache-2.0
(`license.txt`).

`src/esp_lcd_spd2010.c` and `include/esp_lcd_spd2010.h` are copied unmodified.
Upstream gets the `ESP_LCD_SPD2010_VER_*` macros from Espressif's CMake
`cmake_utilities` package; here `library.json` defines them instead.

Waveshare's Arduino 3.1.1 example (github.com/waveshareteam/ESP32-S3-Touch-LCD-1.46,
commit fda89ff2, `example/Arduino-3.1.1/examples/LVGL_Arduino/`) ships a copy
of the same driver. It differs from this one only by a commented-out version
log line and a commented-out `0x3A` (COLMOD) line in the vendor init table,
so the default init sequence is the one Waveshare runs on this board. Issue 22.
