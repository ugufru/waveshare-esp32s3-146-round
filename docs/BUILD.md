# Building and flashing the firmware

Board: Waveshare ESP32-S3-Touch-LCD-1.46 (cover-glass SKU 29565).
Toolchain decision and pinned versions: issue 17. Pin map: issue 19.

## Toolchain

PlatformIO Core with the community **pioarduino** platform, release
`53.03.11` (Arduino-ESP32 **3.1.1** on IDF 5.3.2). The official `espressif32`
platform only ships Arduino 2.x, which Waveshare's SPD2010 display driver does
not support. Every version is pinned by exact URL or version in
`platformio.ini`; do not float them.

The first `pio run` downloads the platform, the Xtensa toolchain and the
Arduino framework (several hundred MB) into `~/.platformio`. An empty sketch
then takes about 2 minutes to build cold.

| Setting | Value | Why |
| --- | --- | --- |
| `board` | `esp32-s3-devkitc-1`, overridden | no Waveshare board definition in the platform |
| `board_build.arduino.memory_type` | `qio_opi` | quad flash + octal PSRAM; the wrong type boot-loops |
| `board_upload.flash_size` | `16MB` | see "Chip" below |
| `board_build.partitions` | `default_16MB.csv` | two 6.25 MB OTA app slots + 3.4 MB SPIFFS |
| `ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1` | | serial over the native USB-Serial/JTAG port |

## Commands

```sh
pio run                  # build
pio run -t upload        # build and flash
pio device monitor       # serial console (Ctrl-C to quit)
```

`pio device monitor` needs a real terminal. Run from Claude Code's `!` prefix
it fails with `termios.error: (19, 'Operation not supported by device')`;
there, read the port non-interactively instead:

```sh
python3 -c "import serial,time;s=serial.Serial('/dev/cu.usbmodem101',115200,timeout=0.5);e=time.time()+12;b=b''
while time.time()<e: b+=s.read(4096)
print(b.decode(errors='replace'))"
```

esptool comes with the platform as the `tool-esptoolpy` package. Run it
through PlatformIO so it uses the right Python and dependencies:

```sh
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 flash_id
```

## First firmware (issue 20)

`src/main.cpp` is a USB serial hello: a banner with chip, flash and PSRAM
sizes, then one line a second with uptime and free heap/PSRAM, repeating the
banner every 10 s so it is visible whenever the monitor is opened. First run
on 2026-10-04 (upload about 13 s):

```
chip ESP32-S3 rev 2, 2 cores @ 240 MHz
flash 16777216 bytes @ 80000000 Hz
psram 8388608 bytes (found)
[80] up 83007 ms, heap free 350632, psram free 8385672
```

If `psram` says NOT FOUND, `board_build.arduino.memory_type` is wrong.

## USB

There is no USB-UART bridge and no reset button. The S3's native USB (GPIO19/20)
goes straight to the USB-C socket and shows up on macOS as
`/dev/cu.usbmodem101`, "Espressif USB JTAG_serial debug unit", VID:PID
`303a:1001`. esptool resets the chip into the ROM bootloader over this port by
itself, so uploads need no buttons as long as the running firmware leaves USB
alone.

**Recovery** if a build wedges USB (port disappears, or upload cannot
connect): hold **BOOT** (GPIO0) while power-cycling the board to enter ROM
download mode, then upload. Both PWR and BOOT are side-actuated switches at the
board edge; in back cup v1.1 they are reached through the pin holes.

## Chip (read 2026-10-04)

```
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
Crystal is 40MHz
MAC: 28:84:85:56:21:4c
Manufacturer: 20   Device: 4018
Detected flash size: 16MB
Flash type set in eFuse: quad (4 data lines)
Flash voltage set by eFuse to 3.3V
```

The flash JEDEC manufacturer is `0x20` (XMC), not `0xEF` (Winbond), so this
unit has an XMC 128 Mbit QSPI part rather than the W25Q128 on the schematic.
Same size and mode, so nothing in the build changes.

## Factory flash backup

Taken on 2026-10-04, before anything was flashed:

```sh
mkdir -p firmware-backup
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 \
    --baud 921600 read_flash 0 0x1000000 firmware-backup/factory-2026-10-04.bin
shasum -a 256 firmware-backup/factory-2026-10-04.bin > firmware-backup/factory-2026-10-04.bin.sha256
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 \
    verify_flash 0 firmware-backup/factory-2026-10-04.bin
```

The read takes about 3.5 minutes. `verify_flash` compared the file with the
chip's own digest and matched.

- File: `firmware-backup/factory-2026-10-04.bin` (16 MB, gitignored; keep a
  copy off this machine)
- sha256: `34b241754137c0f5cfa522bdac513b1bc7264efff9022f849ba72a2d499ae6eb`

Contents, from the partition table at 0x8000:

| Name | Type | Offset | Size |
| --- | --- | --- | --- |
| nvs | data/nvs | 0x009000 | 0x006000 |
| factory | app | 0x010000 | 0x300000 |
| flash_test | data/0x81 | 0x310000 | 0x084000 |
| model | data/spiffs | 0x394000 | 0x5c3000 |

The app is `ESP32-S3-Touch-LCD-1.46-Test` version 1, built Feb 24 2025 with
ESP-IDF `v5.4-dev-2472-gc9df77efbf` (an IDF app, not Arduino). Data ends at
0x5eea0a; the rest of the chip is erased.

Flashing our firmware replaces this partition table with `default_16MB.csv`,
so the factory app cannot be run alongside it. To restore the board exactly as
shipped:

```sh
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32s3 --port /dev/cu.usbmodem101 \
    --baud 921600 write_flash 0 firmware-backup/factory-2026-10-04.bin
```

A second fallback is Waveshare's factory image in the `Firmware` directory of
github.com/waveshareteam/ESP32-S3-Touch-LCD-1.46, also flashed at 0x0.
