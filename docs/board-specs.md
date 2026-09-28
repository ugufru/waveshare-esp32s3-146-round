# Waveshare ESP32-S3-Touch-LCD-1.46: specs and case fit check

Checked 2026-09-28 against the official Waveshare structural drawings (copies in
`docs/datasheets/`) and by slicing the STLs in `hardware/files/`.

## Sources

- Docs: https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.46
- Wiki (1.46B page, same board family): https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46B
- Schematic: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46/ESP32-S3-Touch-LCD-1.46.pdf
- Structural drawing + STEP, with cover glass:
  https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46/ESP32-S3-Touch-LCD-1.46_Structural%20Diagram-With%20Cover.rar
- Structural drawing + STEP, without cover glass (1.46B):
  https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46/ESP32-S3-Touch-LCD-1.46B_Structural%20Diagram-Without%20Cover.rar
- ESP32-S3 datasheet: https://files.waveshare.com/wiki/common/Esp32-s3_datasheet_en.pdf
- QMI8658 IMU: https://files.waveshare.com/wiki/common/QMI8658A.pdf
- PCF85063 RTC: https://files.waveshare.com/wiki/common/PCF85063A.pdf

The STEP models (about 15 MB each) are inside the `.rar` archives and are not
committed. `bsdtar -xf <file>.rar` extracts them on macOS.

## Electrical

| Item | Value |
| --- | --- |
| SoC | ESP32-S3R8, dual-core LX7, up to 240 MHz |
| Memory | 16 MB flash, 8 MB PSRAM, 512 KB SRAM |
| Radio | 2.4 GHz Wi-Fi b/g/n, Bluetooth 5 LE |
| Display | 1.46" round IPS **LCD**, 412 x 412, SPD2010 driver (QSPI) |
| Touch | SPD2010, I2C, TP_INT on GPIO4 |
| IMU | QMI8658, 6-axis |
| RTC | PCF85063 |
| Audio | PCM5101 DAC, onboard mic, speaker connector |
| Storage | TF card slot |
| Power | USB-C, MX1.25 2-pin 3.7 V LiPo connector, charger onboard |
| Buttons | PWR, BOOT |

SKUs: 29565 = 1.46 (with cover glass), 29564 = 1.46B (no cover glass),
33837 = 1.46C.

Note: the Thingiverse README calls it a "1.43-inch 412x412 Round AMOLED". That
is a different Waveshare board (ESP32-S3-Touch-AMOLED-1.43). The 1.46 is an LCD.

## Mechanical (all mm)

| Dimension | 1.46 (cover glass) | 1.46B (no cover) |
| --- | --- | --- |
| Front outline | lens OD 44.77 +/- 0.05 | 39.36 +/- 0.1 BL (41.53 incl. bottom tab) |
| Viewable area | 37.56 +/- 0.15 VA | 37.36 +/- 0.15 VA, 36.96 AA |
| Front glass / LCM | lens 1.60 | LCM 1.51 +/- 0.1 |
| Overall thickness | 10.90 (to header pin tips) | 9.0 |
| Intermediate dim | 7.40 | 5.5 |
| PCB diameter | 39.18 | 39.18 |
| USB-C reach from centre | 23.00 | 23.00 |
| Mounting | 3 x M2 standoffs, 3.5 high | same |

Mounting hole positions (from display centre, PCB side view, USB-C at bottom):
top hole at y = +17.75 on the vertical centreline; bottom-left at
(-12.00, -14.95); bottom-right at (+11.54, -15.45). PCB top edge at +19.64.

## Case as printed (measured from STL)

Case_Top.stl is modelled face-down: z = 0 is the front face.

| Feature | Value |
| --- | --- |
| Outer diameter | 52.0 |
| Wall | 2.6 (bore 46.8) |
| Front plate | 1.0 thick, aperture 41.6 diameter, square edge |
| Cavity depth | 14.0 (z 1.0 to rim at z 15.0) |
| Lid pegs | 3 x 2.5 dia, 2.0 tall, radius 24.7, at 6.5 / 131.5 / -118.8 deg (keyed) |
| USB window | 12.0 wide x 7.0 tall, z 5.6 to 12.6, on the -X axis |
| Vents | none |
| Screw bosses | none |

Case_back.stl (lid): 52.0 dia x 1.8 solid disc with three edge notches for the
pegs. No vents, no holes, no retention feature.

## Fit summary

| Check | Result |
| --- | --- |
| Lens (44.77) in bore (46.8) | fits, **1.0 radial float** |
| Lens on front ledge (41.6 aperture) | 1.58 overlap per side, OK for 1.46 |
| 1.46B (39.36) on 41.6 aperture | **falls through**, case suits 1.46 only |
| Aperture vs viewable area | 41.6 vs 37.56, about 2 mm black border per side, OK |
| Module (10.9) in cavity (14.0) | fits, **about 3.1 axial float** |
| USB-C face (r 23.0) vs bore (r 23.4) | port sits about 3.0 behind the outer skin |
| Rotation of board vs USB window | **not constrained** |
| M2 standoffs used | **no** |
