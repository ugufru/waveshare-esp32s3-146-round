// Board bring-up (issues 20, 21): USB serial hello, shared I2C bus scan,
// TCA9554 expander init and a reset pulse on the touch and LCD reset lines.
#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"

// TCA9554 registers.
enum : uint8_t { TCA_INPUT = 0, TCA_OUTPUT = 1, TCA_POLARITY = 2, TCA_CONFIG = 3 };

static uint8_t tca_addr = 0;  // 0 until found on the bus

static bool i2c_present(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static bool tca_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(tca_addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static int tca_read(uint8_t reg) {
  Wire.beginTransmission(tca_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(tca_addr, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

static void print_info() {
  Serial.printf("chip %s rev %d, %d cores @ %lu MHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("flash %lu bytes @ %lu Hz\n", ESP.getFlashChipSize(),
                ESP.getFlashChipSpeed());
  Serial.printf("psram %lu bytes (%s)\n", ESP.getPsramSize(),
                psramFound() ? "found" : "NOT FOUND");
}

static void i2c_scan(const char *when) {
  Serial.printf("i2c scan (%s):", when);
  int n = 0;
  for (uint8_t a = 0x08; a < 0x78; a++)
    if (i2c_present(a)) { Serial.printf(" 0x%02x", a); n++; }
  Serial.printf("  [%d device%s]\n", n, n == 1 ? "" : "s");
}

static void tca_dump(const char *when) {
  Serial.printf("tca9554 @0x%02x (%s): in 0x%02x out 0x%02x pol 0x%02x cfg 0x%02x\n",
                tca_addr, when, tca_read(TCA_INPUT), tca_read(TCA_OUTPUT),
                tca_read(TCA_POLARITY), tca_read(TCA_CONFIG));
}

static void expander_init() {
  for (uint8_t a = 0x20; a <= 0x27; a++)
    if (i2c_present(a)) { tca_addr = a; break; }
  if (!tca_addr) { Serial.println("tca9554 NOT FOUND in 0x20..0x27"); return; }
  tca_dump("power-on");

  // Resets and SD_CS are outputs, idle high; everything else stays an input.
  const uint8_t outs = (1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST) | (1 << EXIO_SD_CS);
  tca_write(TCA_OUTPUT, 0xFF);   // latch high before switching to output
  tca_write(TCA_CONFIG, (uint8_t)~outs);

  // Pulse both resets: low 20 ms, then give the SPD2010 time to boot.
  tca_write(TCA_OUTPUT, (uint8_t)~((1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST)));
  delay(20);
  tca_dump("resets low");
  tca_write(TCA_OUTPUT, 0xFF);
  delay(200);
  tca_dump("resets released");
}

void setup() {
  Serial.begin(115200);
  // Give the host up to 3 s to open the port so the banner is not lost.
  for (uint32_t t0 = millis(); !Serial && millis() - t0 < 3000;) delay(10);
  Serial.println("\nwaveshare-esp32s3-146-round bring-up");
  print_info();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  i2c_scan("before resets");
  expander_init();
  i2c_scan("after resets");
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("[%lu] up %lu ms, heap free %lu, psram free %lu\n", n++,
                millis(), ESP.getFreeHeap(), ESP.getFreePsram());
  if (n % 10 == 0) {
    print_info();
    i2c_scan("periodic");
    if (tca_addr) tca_dump("periodic");
  }
  delay(1000);
}
