# Pin map and bring-up notes

Board: Waveshare ESP32-S3-Touch-LCD-1.46. Source:
`docs/datasheets/ESP32-S3-Touch-LCD-1.46-schematic.pdf`, page 1 (the GPIO
table in the middle of the sheet and the "IO Extension" block bottom left),
read 2026-10-04. Code uses `include/board_pins.h`, which carries the same
values; change both together.

Tags: **[S]** read from the schematic, **[M]** measured on this board.

## GPIO

| GPIO | Signal | Block | Notes |
| --- | --- | --- | --- |
| 0 | BOOT | key | strapping pin; hold low at power-up for ROM download mode |
| 1 | GPIO1 | header J9 | free |
| 2 | MIC_WS | mic | |
| 3 | GPIO3 | header J9 | free; strapping pin (JTAG source) |
| 4 | TP_INT | touch | |
| 5 | BL_PWM | LCD | backlight, drives the MP1605 boost through Q3 |
| 6 | Key_BAT | power | PWR key sense |
| 7 | BAT_Control | power | power-hold latch (Q1/Q2) |
| 8 | BAT_ADC | power | 200k/100k divider: Vbat = 3 x Vadc |
| 9 | RTC_INT | RTC | PCF85063 interrupt |
| 10 | SCL | I2C | shared bus |
| 11 | SDA | I2C | shared bus |
| 12 | GPIO12 | header J9 | free |
| 13 | GPIO13 | header J9 | free |
| 14 | SD_SCLK | microSD | |
| 15 | MIC_SCK | mic | |
| 16 | SD_MISO | microSD | D0 |
| 17 | SD_MOSI | microSD | CMD |
| 18 | LCD_TE | LCD | tearing effect |
| 19 | USB D- | USB | native USB, do not use |
| 20 | USB D+ | USB | native USB, do not use |
| 21 | LCD_CS | LCD | |
| 33..37 | | PSRAM | "Internal occupancy": octal PSRAM, never touch |
| 38 | I2S_LRCK | audio out | |
| 39 | MIC_SD | mic | |
| 40 | LCD_SCK | LCD | QSPI clock |
| 41 | LCD_SDA3 | LCD | QSPI data 3 |
| 42 | LCD_SDA2 | LCD | QSPI data 2 |
| 43 | UART TXD | header J9 | UART0 |
| 44 | UART RXD | header J9 | UART0 |
| 45 | LCD_SDA1 | LCD | QSPI data 1; strapping pin (VDD_SPI) |
| 46 | LCD_SDA0 | LCD | QSPI data 0; strapping pin (ROM log) |
| 47 | I2S_DIN | audio out | PCM5101 data |
| 48 | I2S_BCK | audio out | |

All of the above is **[S]**. The rows agree with the earlier netlist
extraction recorded in issue 19.

## TCA9554 I/O expander

| Port | Schematic name | Signal | Direction |
| --- | --- | --- | --- |
| P0 | Extend_IO1 | TP_RST | output |
| P1 | Extend_IO2 | LCD_RST | output |
| P2 | Extend_IO3 | SD_CS | output |
| P3 | Extend_IO4 | IMU_INT2 | input |
| P4 | Extend_IO5 | IMU_INT1 | input |
| P5..P7 | Extend_IO6..8 | spare | 10k pull-up to 3V3 |

The schematic numbers the outputs from 1 and the chip from 0, so
**Extend_IOn is port bit n - 1**. `board_pins.h` uses the port bit numbers.
[S]

A1 and A2 are tied to GND and A0 is a 0R/NC option (R50) [S]; the scan finds
the expander at **0x20**, so A0 is low on this board [M]. The expander's INT
output reaches GPIO0 only through R42, which is marked NC: there is no
expander interrupt, so poll the input register. [S]

## I2C bus (IO10 SCL, IO11 SDA)

Scanned at 400 kHz on 2026-10-04 (issue 21), before and after pulsing the
touch and LCD resets, and again every 10 s:

| Address | Device | Status |
| --- | --- | --- |
| 0x20 | TCA9554 expander | found [M] |
| 0x51 | PCF85063 RTC | found [M] |
| 0x6b | QMI8658 IMU | found [M]; SA0 is tied to GND [S], which on the QMI8658 selects 0x6B (the reverse of most parts) |
| 0x53 | SPD2010 touch | found only after display init [M] |

The SPD2010 touch controller does not answer after TP_RST alone (pulsed low
20 ms, scanned at 200 ms and again 10 s later). It appears at 0x53 once the
display side has been reset and initialised over QSPI (issue 22). The SPD2010
is a single display and touch chip: **initialise the display before touch**,
as Waveshare's example does. [M]

Register readback from the expander after init:

```
tca9554 @0x20 (power-on):        in 0xff out 0xff pol 0x00 cfg 0xff
tca9554 @0x20 (resets low):      in 0xfc out 0xfc pol 0x00 cfg 0xf8
tca9554 @0x20 (resets released): in 0xff out 0xff pol 0x00 cfg 0xf8
```

The input register follows the outputs (0xfc while held low), so P0 and P1
really drive their pins. [M]

## Power

Read from the schematic's power section (page 1, top) on 2026-10-07 [S]:

- The cell (BAT) reaches VCC through two P-MOSFETs in series. **Q1**
  (AO3401) has its gate pulled up to BAT by R2 100k, so it is off by default.
  **Q2** (AO3401) has USB_5V on its gate (R12 100k to GND), so it is off
  while USB is present: on USB the cell is disconnected from VCC and only
  charged.
- Q1's gate is pulled low (on) by either the **PWR key** (Key1, through D4)
  or **T1** (8050 NPN) when **BAT_Control (IO7)** is high (R7 1k, R10 10k
  base pull-down). This is the power-hold latch.
- **Key_BAT (IO6)** is pulled up to 3V3 by R1 10k and pulled low through D1
  while PWR is pressed. Active low; no internal pull needed.
- **BAT_ADC (IO8)** is BAT through 200k/100k: Vbat = 3 x Vadc.
- VCC feeds the MP1605 3.3 V buck (U4, EN tied to VIN, 2.3..5.5 V in).
- The **ETA6098** charger's STAT pin only drives the charge LED (LED from BAT
  through R6 3k). The firmware cannot read charge state.

Firmware (src/power.cpp, issue 25):

- `power_begin()` runs first in `setup()`, drives IO7 high on every boot and
  sets `gpio_hold_en` on it so a software reset or panic keeps the latch. On
  battery, PWR must be held until this runs (Arduino start-up is a few
  hundred ms; hold about 1 s).
- Short PWR press: event for the app (the demo toggles the backlight). Held
  2 s: power off. Dropping IO7 turns the board off on battery once PWR is
  released; on USB nothing changes, so after 500 ms the board goes to deep
  sleep with ext0 wake on IO6 low. [M on USB]
- With USB and **no cell**, BAT reads about 4.16 V: that is the charger's
  output with nothing attached, not a battery. [M]

## Bring-up gotchas

- **Resets behind I2C.** The LCD and touch resets are expander outputs, so
  I2C and the TCA9554 must be up before the display can leave reset. Set the
  output latch high before switching the pins to outputs, or they glitch low.
- **Battery power latch.** On battery the board stays on only while
  BAT_Control (IO7) is high. Firmware has to drive it high early in boot or
  the board turns off when PWR is released. Not needed on USB. Issue 25.
- **Strapping pins.** IO45 and IO46 carry LCD data and are also strapping
  pins; IO0 and IO3 are too. Nothing should hold them at a non-default level
  during reset.
- **Touch press state.** Take press/release from the touch point reports
  (weight > 0 / weight 0), not from the status register's pt_exist bit, which
  drops for one read just after every touch starts (issue 24).
- **Display windows.** SPD2010 draw windows need x start and width in
  multiples of 4 pixels (LVGL rounder in src/lvgl_port.cpp).
- **PSRAM pins.** GPIO33..37 belong to the octal PSRAM.
- **Flash.** The schematic says W25Q128; this unit has an XMC part (JEDEC
  manufacturer 0x20, see docs/BUILD.md). Same size and mode.
