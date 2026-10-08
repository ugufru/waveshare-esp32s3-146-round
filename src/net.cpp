#include "net.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_sntp.h>

#include "rtc.h"

static String ssid;
static bool have_creds = false;
static volatile bool synced_flag = false;
static uint32_t last_sync_ms = 0;
constexpr uint32_t SNTP_DELAY_MS = 3000;  // after connect, see start_sntp()
#ifdef NTP_MANUAL_TEST
static const bool sntp_auto = false;
#else
static const bool sntp_auto = true;
#endif

// SNTP callback (runs in the SNTP task): the system clock has just been set.
static void on_time_sync(timeval *) { synced_flag = true; }

static void start() {
  Preferences p;
  p.begin("wifi", true);
  ssid = p.getString("ssid", "");
  const String pass = p.getString("pass", "");
  p.end();
  have_creds = ssid.length() > 0;
  if (!have_creds) {
    Serial.println("net: no Wi-Fi credentials stored (serial: wifi \"<ssid>\" \"<password>\")");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // modem sleep off: see issue 28 (unicast lost on eero)
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("net: connecting to \"%s\"\n", ssid.c_str());
}

// (Re)start SNTP. Called on every Wi-Fi connect: started at boot, before
// the network was up, it did not sync for over a minute after the board
// connected (measured, issue 28). UTC only: the RTC and the system clock stay
// in UTC (issue 26), and the UI adds the stored offset.
static void start_sntp() {
  // configTime takes the TCPIP core lock and stops a running SNTP itself.
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}
static uint32_t sntp_due = 0;  // millis() when to start SNTP, 0 = not pending

void net_begin() {
  sntp_set_time_sync_notification_cb(on_time_sync);
  start();
}

void net_loop() {
  static wl_status_t last = WL_IDLE_STATUS;
  const wl_status_t st = WiFi.status();
  if (st != last) {
    last = st;
    if (st == WL_CONNECTED) {
      net_print_status();
      if (sntp_auto) sntp_due = millis() + SNTP_DELAY_MS;
    }
    else if (have_creds) Serial.printf("net: Wi-Fi status %d\n", (int)st);
  }
  if (sntp_due && (int32_t)(millis() - sntp_due) >= 0) {
    sntp_due = 0;
    start_sntp();
    Serial.println("net: SNTP started");
  }
  if (synced_flag) {
    synced_flag = false;
    const time_t now = time(nullptr);
    const uint32_t gap = last_sync_ms ? (millis() - last_sync_ms) / 1000 : 0;
    last_sync_ms = millis();
    const bool ok = rtc_set(now, rtc_offset_min());
    Serial.printf("net: NTP sync %lld UTC, RTC %s (offset %+d min)%s\n", (long long)now,
                  ok ? "updated" : "write FAILED", rtc_offset_min(),
                  gap ? (String(", ") + gap + " s since last sync").c_str() : "");
  }
}

void net_set_credentials(const char *s, const char *pass) {
  Preferences p;
  p.begin("wifi", false);
  p.putString("ssid", s);
  p.putString("pass", pass);
  p.end();
  Serial.printf("net: stored credentials for \"%s\" (password %u chars)\n", s, (unsigned)strlen(pass));
  WiFi.disconnect(true);
  delay(100);
  start();
}

void net_clear_credentials() {
  Preferences p;
  p.begin("wifi", false);
  p.clear();
  p.end();
  WiFi.disconnect(true);
  have_creds = false;
  ssid = "";
  Serial.println("net: credentials cleared, Wi-Fi off");
}

void net_print_status() {
  if (!have_creds) { Serial.println("net: no credentials stored"); return; }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("net: not connected to \"%s\" (status %d)\n", ssid.c_str(), (int)WiFi.status());
    return;
  }
  Serial.printf("net: connected to \"%s\" (bssid %s), %s/%s gw %s, mac %s, %d dBm, channel %d, last NTP sync %s\n",
                ssid.c_str(), WiFi.BSSIDstr().c_str(), WiFi.localIP().toString().c_str(),
                WiFi.subnetMask().toString().c_str(), WiFi.gatewayIP().toString().c_str(),
                WiFi.macAddress().c_str(), WiFi.RSSI(), WiFi.channel(),
                last_sync_ms ? (String((millis() - last_sync_ms) / 1000) + " s ago").c_str() : "not yet");
}

const char *net_ui_status() {
  static char buf[24];
  if (!have_creds) return "no wifi";
  if (WiFi.status() != WL_CONNECTED) return "wifi ...";
  snprintf(buf, sizeof buf, "wifi %d dBm%s", WiFi.RSSI(), last_sync_ms ? "" : ", no ntp");
  return buf;
}

void net_print_ntp() {
  // No WiFi.hostByName() here: in Arduino-ESP32 3.1.1 it clears the lwIP DNS
  // cache without the TCPIP core lock, which runs SNTP's pending DNS callback
  // unlocked and asserts in udp_new_ip_type (crash seen, issue 28).
  Serial.printf("net: sntp enabled %d, sync status %d, time %lld, dns %s / %s\n", esp_sntp_enabled(),
                (int)sntp_get_sync_status(), (long long)time(nullptr),
                WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());
  for (int i = 0; i < 3; i++) {
    const char *name = esp_sntp_getservername(i);
    Serial.printf("net: server %d %s reach 0x%02x\n", i, name ? name : "(none)", esp_sntp_getreachability(i));
  }
}

// One NTP request over a plain UDP socket (diagnostic, serial: ntptest).
// Returns the server's transmit time as Unix seconds, or 0 on failure.
static time_t ntp_query(const char *host, IPAddress ip) {
  WiFiUDP udp;
  if (!udp.begin(0)) { Serial.println("net: udp begin failed"); return 0; }
  uint8_t pkt[48] = {0x23};  // LI 0, version 4, mode 3 (client)
  udp.beginPacket(ip, 123);
  udp.write(pkt, sizeof pkt);
  udp.endPacket();
  const uint32_t t0 = millis();
  while (millis() - t0 < 2000) {
    if (udp.parsePacket() >= 48) {
      udp.read(pkt, sizeof pkt);
      udp.stop();
      const uint32_t secs = (uint32_t)pkt[40] << 24 | pkt[41] << 16 | pkt[42] << 8 | pkt[43];
      Serial.printf("net: ntp %s (%s): reply in %lu ms, %lu\n", host, ip.toString().c_str(),
                    millis() - t0, (unsigned long)(secs - 2208988800UL));
      return secs - 2208988800UL;
    }
    delay(5);
  }
  udp.stop();
  Serial.printf("net: ntp %s (%s): no reply in 2 s\n", host, ip.toString().c_str());
  return 0;
}

void net_ntp_test() {
  if (esp_sntp_enabled()) {
    Serial.println("net: ntptest needs SNTP not running (build with -DNTP_MANUAL_TEST)");
    return;
  }
  for (const char *h : {"pool.ntp.org", "time.google.com"}) {
    IPAddress ip;
    if (WiFi.hostByName(h, ip)) ntp_query(h, ip);
    else Serial.printf("net: dns %s FAILED\n", h);
  }
  ntp_query("google literal", IPAddress(216, 239, 35, 0));
}

// Diagnostic: drop the association, wait, reconnect (serial: wifi reconnect).
void net_reconnect(uint32_t wait_ms) {
  WiFi.disconnect(true, false);
  Serial.printf("net: disconnected, waiting %lu ms\n", (unsigned long)wait_ms);
  delay(wait_ms);
  start();
}
