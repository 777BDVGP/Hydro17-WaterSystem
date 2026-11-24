// =============================================================================
// 05_WiFi.h — WiFi join (saved / built-in networks) + setup hotspot
// =============================================================================
// Ported from RAINGAUGE/RainGauge_PIO and Prima3_pomphuis (2026-11-24).
// SetupWiFi() (called once from setup()):
//
//   1. BOOT (GPIO 0) held LOW, or CONTROLLER DIS/EN (GPIO 34) held HIGH, at
//      power-on -> setup hotspot "Hydro17-Setup" (open, http://192.168.4.1/,
//      captive DNS, 5 min then reboot).
//   2. Nothing saved and no built-in networks (public firmware, new board)
//      -> setup hotspot, no time limit.
//   3. /WiFiConfig.json holds a LIST of networks (max 8, newest first):
//      scan, try the saved ones in range (strongest first), then the rest.
//   4. Private firmware only: scan for the KNOWN_NETS from wifi_secrets.h,
//      join the strongest on its fixed IP and ADD it to /WiFiConfig.json.
//   5. Every join scans all channels and picks the STRONGEST access point of
//      the SSID (mesh / multi-AP) — logged with its BSSID in the General Log.
//   6. Steps 3-4 are tried WIFI_ROUNDS times; if nothing joins, the setup
//      hotspot opens for 5 minutes, then the controller reboots and retries.
//
// /wifi page "WiFi Networks" lists the saved networks (never the passwords),
// with DELETE and ADD.  The old /WifiSettings.json is imported into the list
// once (first start of this firmware) and then deleted.
// =============================================================================

#pragma once
#include <Arduino.h>

// Filled in by SetupWiFi() — shown on the /wifi page.
	String WiFiSource = "";   // "saved" or "built-in"

bool SetupWiFi();
