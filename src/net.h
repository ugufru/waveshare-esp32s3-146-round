// Wi-Fi station and network time (issue 28). Credentials live in NVS
// (namespace "wifi"), set over serial, never in source or git.
#pragma once

#include <stdint.h>

// Starts Wi-Fi if credentials are stored; NTP then keeps the system clock
// and the RTC in UTC (the RTC's local offset is left as it is).
void net_begin();

// Call from loop(): logs connection changes.
void net_loop();

// Store credentials and (re)connect. The password is never printed.
void net_set_credentials(const char *ssid, const char *pass);
void net_clear_credentials();

// Drop the association, wait, reconnect (serial: wifi reconnect).
void net_reconnect(uint32_t wait_ms);

// One-line status for serial ("connected to <ssid>, 192.168.1.5, -55 dBm").
void net_print_status();

// SNTP diagnostics: sync status, server reachability, DNS (serial: ntp).
void net_print_ntp();

// Plain-UDP NTP test, SNTP not running (serial: ntptest).
void net_ntp_test();

// Short status for the UI, e.g. "wifi -55 dBm", "wifi ...", "no wifi".
const char *net_ui_status();
