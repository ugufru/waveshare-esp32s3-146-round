// Circuit (schematic page 1, power section): the battery reaches VCC through
// P-MOSFET Q1 (gate pulled up to BAT, so off) and Q2 (off while USB_5V is
// present). Q1's gate is pulled low, turning it on, by the PWR key (through
// D4) or by T1 when BAT_Control (IO7) is high. The key also pulls Key_BAT
// (IO6, 10k to 3V3) low through D1. BAT_ADC (IO8) is BAT through 200k/100k.
// The ETA6098 charger's STAT pin only drives the charge LED: charge state is
// not readable.
#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "board_pins.h"

constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t LONG_MS = 2000;
constexpr uint32_t RELEASE_WAIT_MS = 500;

static BootSource boot_source = BootSource::Usb;

void power_begin() {
  // Latch on every boot, not only when PWR started us (Waveshare's example
  // latches only then), so unplugging USB with a cell fitted does not reset.
  pinMode(PIN_BAT_CONTROL, OUTPUT);
  digitalWrite(PIN_BAT_CONTROL, HIGH);
  // Keep the pad driven through a software reset or panic reboot; otherwise
  // R10 pulls T1 off and a crash on battery is a power-off.
  gpio_hold_en((gpio_num_t)PIN_BAT_CONTROL);

  pinMode(PIN_KEY_BAT, INPUT);  // external 10k pull-up
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) boot_source = BootSource::Wake;
  else if (digitalRead(PIN_KEY_BAT) == LOW) boot_source = BootSource::PowerKey;
  else boot_source = BootSource::Usb;

  // 12-bit, default 11 dB attenuation (0..~3.1 V; Vbat/3 is at most 1.4 V).
  // analogSetPinAttenuation() before the first read logs "Pin is not
  // configured as analog channel", so it is not called.
  analogReadResolution(12);
}

BootSource power_boot_source() { return boot_source; }

float power_battery_volts() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BAT_ADC);
  return mv / 16.0f * 3.0f / 1000.0f;
}

int power_battery_percent(float v) {
  // Resting single-cell LiPo, approximate. Under load or on charge it reads
  // high or low; good enough for a gauge, not for a fuel gauge.
  static const struct { float v; int pct; } curve[] = {
      {4.20f, 100}, {4.10f, 90}, {4.00f, 80}, {3.90f, 65}, {3.80f, 50},
      {3.75f, 40},  {3.70f, 30}, {3.65f, 20}, {3.50f, 10}, {3.30f, 0}};
  if (v >= curve[0].v) return 100;
  for (size_t i = 1; i < sizeof curve / sizeof curve[0]; i++)
    if (v >= curve[i].v)
      return curve[i].pct + (int)((v - curve[i].v) / (curve[i - 1].v - curve[i].v) *
                                  (curve[i - 1].pct - curve[i].pct));
  return 0;
}

KeyEvent power_key_poll() {
  static bool stable = false, last_raw = false, long_sent = false;
  static uint32_t changed = 0, pressed_at = 0;
  // Ignore the press that powered us on until it has been released once.
  static bool armed = boot_source != BootSource::PowerKey && boot_source != BootSource::Wake;

  const bool raw = digitalRead(PIN_KEY_BAT) == LOW;
  const uint32_t now = millis();
  if (raw != last_raw) { last_raw = raw; changed = now; }
  if (now - changed < DEBOUNCE_MS || raw == stable) {
    if (stable && armed && !long_sent && now - pressed_at >= LONG_MS) {
      long_sent = true;
      return KeyEvent::LongHeld;
    }
    return KeyEvent::None;
  }
  stable = raw;
  if (stable) {
    pressed_at = now;
    long_sent = false;
    return KeyEvent::None;
  }
  // Released.
  if (!armed) { armed = true; return KeyEvent::None; }
  return long_sent ? KeyEvent::None : KeyEvent::Short;
}

void power_off() {
  gpio_hold_dis((gpio_num_t)PIN_BAT_CONTROL);
  digitalWrite(PIN_BAT_CONTROL, LOW);
  // The key itself still holds Q1 on: wait for release.
  while (digitalRead(PIN_KEY_BAT) == LOW) delay(10);
  delay(RELEASE_WAIT_MS);
  // Still here, so VCC comes from USB. Sleep until PWR is pressed.
  Serial.println("power: still powered (USB), deep sleep until PWR");
  Serial.flush();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_KEY_BAT, 0);
  esp_deep_sleep_start();
}
