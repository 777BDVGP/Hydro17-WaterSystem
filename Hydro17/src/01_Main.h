// =============================================================================
// 01_Main.h — Hardware Platform, Libraries & Global Variables
// =============================================================================
// Defines the board version, includes all third-party libraries, declares the
// platform I/O pin aliases (LED, reset button), watchdog variables, and the
// pre-boot LED ticker used by the captive-portal phase in 05_WiFi.cpp.
// =============================================================================

// ─── Runtime Object Presence Flags ───────────────────────────────────────────
// Set to true once the corresponding page-variable list has been allocated.
// Guards in MaintainWebPageValues() skip maintenance until the page is built.
	bool MainPageObject       = false;  // Home page PageVar list allocated
	bool SuspendObject        = false;  // Suspend page PageVar list allocated
	bool ElectricValvesObject = false;  // Valve pages PageVar lists allocated
	bool TankPageObject       = false;  // Tank page PageVar list allocated

// ─── Board Version ────────────────────────────────────────────────────────────
	#define BRD_VERSION 1

// ─── Libraries ────────────────────────────────────────────────────────────────
	#include <WiFi.h>                 // Core WiFi (explicit — replaces WiFiManager)
	#include <WiFiUdp.h>
//	#include <FS.h>                   // Files System
 	#include <SPIFFS.h>               // Files System
 	#include <Wire.h>	              // OLED Display
	#include <ArduinoJson.h>          // JSON file system serialisation
	#include <ESPAsyncWebServer.h>
	#include <Ticker.h>               // Watchdog & LED flash ticker
 	#include <time.h>                 // ESP32 built-in NTP/time (replaces ESPPerfectTime)
	#include <HTTPClient.h>           // NTFY.sh push notifications
	#include "ping/ping_sock.h"       // IDF esp_ping — gateway keepalive (MaintainGatewayWatch)

 	#include <Adafruit_SSD1306.h>     // OLED Display
	#include "EmonLib.h"              // Current Transformer Library

// ─── Board-Specific Pin Aliases ───────────────────────────────────────────────
// ESP8266 uses Dx labels; ESP32-S2 uses raw GPIO numbers.
	#if defined(ARDUINO_ARCH_ESP8266)
		#define ASPRESET D3    // GPIO 0  — WiFiManager reset button (D3)
		#define LEDOUT   D4    // GPIO 2  — PCB activity LED (D4)
	#elif defined(ARDUINO_ARCH_ESP32)
		#define ASPRESET 00    // GPIO 0  — PCB Button 0     (Button 0 on ESP32-S2 board)
		#define LEDOUT   15    // GPIO 15 — PCB activity LED (Blue LED on ESP32-S2 board)
	#endif

// ─── Watchdog Variables ───────────────────────────────────────────────────────
// secondTick fires ISRwatchdog() every 1 s; MaintainTimeCheck() (loop) resets watchdogCount.
// If the count exceeds Params->WatchdogTime (seconds) the firmware forces a reboot.
	Ticker secondTick;
	volatile int watchdogCount = 0; // Incremented by ISR every second, reset by MaintainTimeCheck() in loop()
	String localIP;                 // Cached local IP address string (set after WiFi connect)

// ─── Gateway Keepalive / Reachability Guard (MaintainGatewayWatch, 01_Main.cpp) ─
// WiFi.status() alone can keep reporting WL_CONNECTED while the device has
// silently dropped off the network (no ping, no ARP reply). Pinging the
// router proves the link actually carries traffic; reboot if it goes silent.
// The ping ALSO acts as a KEEPALIVE: regular outbound traffic stops the Omada
// AP/switch/router from ageing this client out (see main.cpp note). Keep the
// interval well under the network's idle/ARP ageing times.
// Always pings the gateway; on/off is Params->KeepAliveEnabled (/settings).
	#define GW_PING_INTERVAL_MS   30000UL    // One ping to the target every 30 s
	#define GW_PING_TIMEOUT_MS     2000UL    // Per-ping reply timeout
	#define GW_UNREACHABLE_MS    300000UL    // Reboot after 5 min with no reply

// ─── Pre-Boot LED Ticker ──────────────────────────────────────────────────────
// MsTick is attached by 05_WiFi.cpp during the captive-portal phase to flash
// the LED.  Declared here so both TUs can reference it.
    Ticker MsTick;

// ─── NTFY.sh Push Notifications ───────────────────────────────────────────────
// NtfyMsg struct and NtfyParams class are declared in 16_NTFY.h.
