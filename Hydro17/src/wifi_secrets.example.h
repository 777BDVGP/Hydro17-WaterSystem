#pragma once

// =============================================================================
// wifi_secrets.example.h — OPTIONAL template for wifi_secrets.h
// =============================================================================
// You do NOT need this file: without wifi_secrets.h the controller opens the
// WiFi hotspot "Hydro17-Setup" and you choose your network on a phone.
//
// Only for networks built into the firmware: copy this file to wifi_secrets.h
// (same folder) and fill it in.  wifi_secrets.h is in .gitignore, so it never
// goes into Git.  Each entry: SSID, password, fixed IP the controller takes.
// =============================================================================

	#define WIFI_KNOWN_NETS \
		{ "MyHomeWiFi",  "my-password",  { 192, 168, 1, 120 } },
