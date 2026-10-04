// USB serial hello (issue 20): proves build, flash and monitor on this board.
// Prints chip, flash and PSRAM sizes and free heap once a second over the
// native USB-Serial/JTAG port.
#include <Arduino.h>

static void print_info() {
  Serial.printf("chip %s rev %d, %d cores @ %lu MHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("flash %lu bytes @ %lu Hz\n", ESP.getFlashChipSize(),
                ESP.getFlashChipSpeed());
  Serial.printf("psram %lu bytes (%s)\n", ESP.getPsramSize(),
                psramFound() ? "found" : "NOT FOUND");
}

void setup() {
  Serial.begin(115200);
  // Give the host up to 3 s to open the port so the banner is not lost.
  for (uint32_t t0 = millis(); !Serial && millis() - t0 < 3000;) delay(10);
  Serial.println("\nwaveshare-esp32s3-146-round hello");
  print_info();
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("[%lu] up %lu ms, heap free %lu, psram free %lu\n", n++,
                millis(), ESP.getFreeHeap(), ESP.getFreePsram());
  if (n % 10 == 0) print_info();
  delay(1000);
}
