// =============================================================================
// main.cpp — Boot Sequence, FreeRTOS Task Launch & Hardware Test Utilities
// =============================================================================
//
// Platform  : WEMOS WEMOS_ESP32_2S_V5a (ESP32-S2, 240 MHz, 4 MB Flash)
// Framework : Arduino / FreeRTOS  (PlatformIO)
//
// Description:
//   Automated water management controller for a residential water
//   system (Hydro17).  Manages nine irrigation zones, borehole pump and tank
//   supply solenoid, tank-level monitoring (float switch or pressure sensor),
//   pool filter pump scheduling, and solenoid-valve configurations.
//   All control logic runs from the Arduino loop() (single execution
//   context — see "WHY loop() AND NOT FreeRTOS TASKS" below).  A real-time web interface
//   (HTTP + WebSocket on port 80) provides live status and full configuration
//   without requiring a mobile app or cloud connection.
//
// Key Features:
//   - 9-zone irrigation scheduler with per-zone frequency, duration,
//     water-source selection (tank or borehole), and day-of-week suspension
//   - Automatic tank filling via float switches or pressure sensor, with
//     configurable start/stop level and optional scheduled fill window
//   - Borehole pump control with tank supply solenoid sequencing
//   - 6 user solenoid valves across 9 named configurations + tank-fill config
//   - Pool filter pump with 7-day schedule and manual override
//   - Pool light relay with manual web control
//   - 7-day irrigation suspension calendar
//   - SPIFFS-backed JSON persistent storage for all settings and schedules
//   - General, irrigation and tank-fill event logs (auto-trimmed daily)
//   - WiFi profile auto-switching (/WifiProfiles.json) — selects static IP
//     from the connected network's profile (/WifiSettings.json)
//   - ArduinoOTA for wireless firmware updates
//   - Hardware watchdog with automatic reboot on task stall
//
// loop() schedule:
//   OTA active   : ArduinoOTA.handle() only, ~1 kHz
//   every 10 ms  : MaintainTimeCheck (02_Time.cpp)
//                    watchdogCount reset, LEDToggle, MaintainTimers,
//                    PinDebounce, MaintainMidnightReboot,
//                    MaintainGatewayWatch (keepalive — see note below),
//                    heap + WiFi watchdogs every 15 s
//                  MaintainSystem (05_ThisSystem.cpp)
//                    MaintainValveStatus, IrrigationShedule,
//                    MaintainTankFilling, MaintainController,
//                    MaintainPoolFilterPump
//   every pass   : MaintainWebService (04_WebPages.cpp, self-timed:
//                    OTA poll 200 ms / web page 50 ms / home page 500 ms)
//                  delay(1) — yields to AsyncTCP / WiFi / IDLE
//   NtfyTask (16_NTFY.cpp) is the only remaining FreeRTOS task — it does a
//   blocking HTTPClient POST that would stall control logic if run in loop().
//
// -----------------------------------------------------------------------------
// WHY loop() AND NOT FreeRTOS TASKS (changed 2026-09-27)
// -----------------------------------------------------------------------------
// The firmware used to run SystemTask, WebSocketTask and TimeCheckTask, all
// pinned to core 0 (the S2 is single-core anyway), each a
// `for(;;) { work(); vTaskDelay(10ms); }`. That was already cooperative
// round-robin — exactly what one loop() does — but cost 16 KB of heap for
// three task stacks, while loop()'s own 8 KB stack sat idle. On the S2 that
// heap is what the web pages need: beginHtmlStream() (04_WebPages.h) answers
// "Low memory — retrying" whenever free heap < 18 KB or the largest block
// < 10 KB. Folding the tasks into loop() returns that RAM and removes the
// unguarded cross-task access to OutPort[]/InPort[]/Params/PageVar lists.
// Same change as Prima3_pomphuis.
//
// Boot sequence (setup()):
//   1. Serial init (115 200 baud)
//   2. InitHardware()            — GPIO setup
//   3. InitFileSystem()          — SPIFFS mount, operational object creation
//   4. SetupWiFi()               — STA connect from /WifiSettings.json, or captive portal
//   5. SetupTime()               — NTP sync (SAST UTC+2, za.pool.ntp.org)
//   6. SetupWebPage()            — HTTP routes + WebSocket
//   7. IrrigationInit()          — repair stale schedule dates
//   8. SaveAboutIP()             — persist connected IP into About.json for display
//   9. SetupOTA()                — start ArduinoOTA service
//  10. WatchdogSetup()           — arm 1-second Ticker ISR
//  11. SetupNTFY()               — NTFY queue + NtfyTask; then loop() runs
//
// WiFi Reset (clears /WifiSettings.json → captive portal on next boot):
//   - Hold CONTROLLER DIS/EN button (GPIO 34) HIGH during power-on
//   - Hold ASPRESET (GPIO 0) LOW during power-on
//   - Hold ASPRESET (GPIO 0) LOW for 5 s while running
//   - Click "RESET ASP" on the About web page (/setASP)
//
// Build Settings (PlatformIO):
//   Partition Scheme  : Huge App (3 MB App / 1 MB SPIFFS)
//   USB CDC on Boot   : Enabled
//   USB Firmware MSC  : Disabled
//   USB DFU on Boot   : Disabled
//   Core Debug Level  : None
//
// -----------------------------------------------------------------------------
// GATEWAY KEEPALIVE — "UNREACHABLE AFTER A WHILE" (added 2026-09-27)
// -----------------------------------------------------------------------------
// Symptom: a unit on a TP-Link Omada network becomes
// unreachable after a while — it answers neither ping nor ARP, yet all tasks
// keep running and WiFi.status() still reports WL_CONNECTED, so neither the
// software watchdog nor the WiFi-lost guard in TimeCheckTask recovers it.
// Prima3_pomphuis (ESP32-S3, also on Omada) had exactly the same fault and it
// stopped once the ESP started pinging the router every 30 s. Since this unit
// is an S2 on a different (older) core and fails the same way, the Omada
// network — not the chip or IDF version — is the common factor.
//
// Working theory: Omada APs/switches age silent clients out of their client /
// MAC tables, and ARP for this IP then stops reaching the ESP. Regular
// outbound traffic keeps the AP table, switch MAC table and router ARP entry
// fresh.
//
// MaintainGatewayWatch() (01_Main.cpp, called from TimeCheckTask) therefore:
//   - pings the gateway (router) every 30 s — KEEPALIVE
//   - reboots after 5 min with no reply — SAFETY NET
//   - logs WiFi disconnect reason codes and the reboot reason to GeneralLog
// On/off checkbox is on the Settings page (/settings).
// DO NOT remove MaintainGatewayWatch() as "just diagnostics" — it is both the
// keepalive and the reboot safety net.
//
// GeneralLog lines to look for:
//   "REBOOT - Gateway unreachable 5 min (missed N, status S, RSSI R, heap H,
//    min heap M)"  — status 3 = WL_CONNECTED confirms the "zombie" state.
//   "WiFi disconnected, reason N (drops since boot: K)" — N is an IDF
//    wifi_err_reason_t (8 ASSOC_LEAVE, 15 4WAY_HANDSHAKE_TIMEOUT,
//    200 BEACON_TIMEOUT, 201 NO_AP_FOUND).
// No "Gateway unreachable" lines over a week+ = keepalive is doing its job.
// Reference: Prima3_pomphuis src/main.cpp investigation note.
//
// =============================================================================

#include <Arduino.h>

// Firmware version — shown on ABOUT, compared by CHECK FOR UPDATE. Bump it with every release.
	#define FW_VERSION "2.0.2"

	#define DEBUG 0  // Set to 1 to see output, 0 for Production 

	#if DEBUG
	  #define DEBUG_PRINT(x) Serial.print(x)
	  #define DEBUG_PRINTLN(x) Serial.println(x)
	  #define DEBUG_PRINTF(x, ...) Serial.printf(x, ##__VA_ARGS__)
	#else
	  #define DEBUG_PRINT(x)
	  #define DEBUG_PRINTLN(x)
	  #define DEBUG_PRINTF(x, ...)
	#endif

	#define FORMAT_NOT           0   // Normal boot — do not touch SPIFFS
	#define FORMAT_YES           1   // Force a full SPIFFS format on next boot
	#define SPIFFS_MODE          FORMAT_NOT  // ← change to FORMAT_YES, upload, then restore

	#define DEV_MODE_NORMAL      0   // All FreeRTOS tasks run; loop() is idle
	#define DEV_MODE_TEST_OUT    1   // No tasks; loop() calls TestOutPins()
	#define DEV_MODE_TEST_IN     2   // No tasks; loop() calls TestInPins()
	#define DEV_MODE             DEV_MODE_NORMAL
// 	#define DEV_MODE             DEV_MODE_TEST_OUT 
// 	#define DEV_MODE             DEV_MODE_TEST_IN

	#include "01_Main.h"
	#include "02_Time.h"
	#include "03_Files.h"
	#include "05_WiFi.h"
	#include "04_WebPages.h"
	#include "05_ThisSystem.h"
	#include "08_Home.h"
	#include "09_Suspend.h"
	#include "09_WaterValves.h"
	#include "09_Irrigation.h"
	#include "11_Other.h"
	#include "12_AnalogRead.h"
	#include "12_Tank.h"
	#include "13_Pool.h"
	#include "14_Plugs.h"
	#include "12_Settings.h"
	#include "15_About.h"
	#include "16_NTFY.h"
	#include "17_RainGauge.h"
	#include "22_Update.h"

// ─── Default Static IP (/wifi edit page only) ────────────────────────────────
// Pre-fills the /wifi form when /WifiSettings.json is missing or the entered
// IP is invalid (12_Settings.cpp). Gateway/subnet/DNS are derived from it.
// The IP actually used at boot comes from /WifiSettings.json, and the house
// is selected at runtime via /WifiProfiles.json (05_WiFi.cpp) — not here.
	#if DEBUG
		#define NAV_IP   IPAddress(192,168,1,121)   // DEV   — test box
	#else
		#define NAV_IP   IPAddress(192,168,3,120)   // PROD
	#endif

	// Forward declarations (replaces Arduino IDE's auto-generated prototypes)
	#include "00_ForwardDecl.h"

	unsigned int __bss_end;
	unsigned int __heap_start;
	void *__brkval;

// ─── FreeRTOS Task Handle (NtfyTask is the only task — see header) ───────────
TaskHandle_t NtfyTaskTaskHandle = NULL;    // 16_NTFY.cpp
QueueHandle_t NtfyQueue = NULL;

// ─────────────────────────────────────────────────────────────────────────────
// setup
// One-time board initialisation, WiFi provisioning and service start-up.
// Execution sequence:
//   1. Serial + hardware I/O initialisation
//   2. SPIFFS file system mount and object creation
//   3. SetupWiFi() — STA connect from /WifiSettings.json or captive portal AP
//   4. NTP time sync, web server routes, OTA, irrigation schedule initialisation
//   5. Watchdog armed, NtfyTask started, then setup() returns and loop() runs
// ─────────────────────────────────────────────────────────────────────────────
void setup(){
	// StartSerial Comms
    #if DEBUG
		Serial.begin(115200); delay(5000); DEBUG_PRINTLN("\nBoot-Up Hydro 17 Water System");
 	#endif

	// Init Board's IOs
	InitHardware();
	// SaveGenReport("Heap after InitHardware:         " + String(ESP.getFreeHeap()) + " B");

	// ── Optional SPIFFS Format ────────────────────────────────────────────────
	// Set SPIFFS_MODE to FORMAT_YES, upload, then restore to FORMAT_NOT.
	// Uses #if (not #ifdef) so the value is compared, not just checked for existence.
	#if SPIFFS_MODE == FORMAT_YES
		DEBUG_PRINTLN("*** SPIFFS FORMAT REQUESTED — erasing partition...");
		SPIFFS.begin();
		if (SPIFFS.format()) { DEBUG_PRINTLN("*** SPIFFS format OK"); }
		else                 { DEBUG_PRINTLN("*** SPIFFS format FAILED"); }
		SPIFFS.end();
	#endif

 	// Setup Memory from Files
 	InitFileSystem();
	// SaveGenReport("Heap after InitFileSystem:        " + String(ESP.getFreeHeap()) + " B");

	// Initiating of Operational Objects
	InitOperationalObjects();
	// SaveGenReport("Heap after InitOperationalObjects:" + String(ESP.getFreeHeap()) + " B");

	// Init Other Options
	InitOther(); // Update Timer Times

	// Connect to WiFi — GPIO 34 HIGH or GPIO 0 LOW at boot triggers portal reset.
	// SetupWiFi() returns false only if STA connect times out; runCaptivePortal()
	// never returns (it restarts after the user saves credentials).
	if (!SetupWiFi()) {
		SaveGenReport("REBOOT - WiFi connect failed");
		delay(500);
		ESP.restart();
	}
	else {
		MsTick.detach();
		DEBUG_PRINTLN("Server has been Connected!");
		SetupTime();	  // Initiate Internet Time — waits up to 10 s for NTP sync
		// SaveGenReport("Heap after SetupTime:             " + String(ESP.getFreeHeap()) + " B");
		SetupWebPage();	  // Set Routes, Start Server & Start Web Socket
		// SaveGenReport("Heap after SetupWebPage:          " + String(ESP.getFreeHeap()) + " B");
		IrrigationInit(); // Update Scheduled Times in the rear
		SaveAboutIP(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), WiFi.dnsIP());
		SetupOTA();       // Start ArduinoOTA service
		// SaveGenReport("Heap after SetupOTA:              " + String(ESP.getFreeHeap()) + " B");
	}

	IPAddress locIP = WiFi.localIP();
	localIP = locIP.toString();
	DEBUG_PRINTLN("Start Loop");
	#if DEV_MODE == DEV_MODE_NORMAL
  		WatchdogSetup();
	#endif

    if(!Params->NormalOperationSaved)
	{
		SwitchNormalOperation();
		DEBUG_PRINTLN("NormalOperation set to FALSE ");
	}

	// ── Start services — all control logic runs from loop() ───────────────────
	#if DEV_MODE == DEV_MODE_NORMAL
		SetupNTFY();   // NTFY queue + NtfyTask
		SetupRainGauge();   // rain-gauge queues + RainGaugeTask (17_RainGauge.cpp)
		// Logged last so the NTP date is valid and the heap figures are the
		// baseline loop() starts with (compare against "Low memory" limits:
		// free 18 KB / maxBlock 10 KB, beginHtmlStream in 04_WebPages.h).
		SaveGenReport("Up and Running after REBOOT - free=" + String(ESP.getFreeHeap()) + " maxBlock=" + String(ESP.getMaxAllocHeap()) + " B, PSRAM free=" + String(ESP.getFreePsram()) + " B");
	#else
		DEBUG_PRINT("DEV_MODE = "); DEBUG_PRINTLN(DEV_MODE); DEBUG_PRINTLN("Test mode — control logic NOT running");
	#endif
}

void TestOutPins();
void TestInPins();

// ─────────────────────────────────────────────────────────────────────────────
// loop
// Single execution context for all control and web upkeep (schedule in the
// file header). In TEST modes it drives the I/O test function instead.
// ─────────────────────────────────────────────────────────────────────────────
void loop(){
	#if DEV_MODE == DEV_MODE_TEST_OUT
		TestOutPins();
	#elif DEV_MODE == DEV_MODE_TEST_IN
		TestInPins();
	#else
		// During an OTA flash do nothing but pump the update stream — control
		// logic and web work would compete with it on the single-core S2.
		if (OTAInProgress){
			ArduinoOTA.handle();
			delay(1);
			return;
		}

		static uint32_t t10 = 0;
		uint32_t now = millis();
		if (now - t10 >= 10){
			t10 = now;
			MaintainTimeCheck();     // Watchdog, LED, timers, debounce, keepalive, heap/WiFi guards (02_Time.cpp)
			MaintainSystem();        // Valves, irrigation, tank, controller, pool pump            (05_ThisSystem.cpp)
		}

		MaintainWebService();        // OTA poll + web/WebSocket upkeep, self-timed                  (04_WebPages.cpp)

		delay(1);                    // Yield to AsyncTCP / WiFi / IDLE task
	#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// MonitorStackAndHeap  (development helper — not currently called)
// Prints the current FreeRTOS stack high-water mark and minimum free heap to
// Serial.  Useful for diagnosing stack overflows or heap exhaustion during
// development; the timer is currently commented out in loop().
// ─────────────────────────────────────────────────────────────────────────────
void MonitorStackAndHeap(){

	UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	//Serial.println("Stack high water mark: " + String(uxHighWaterMark));
	//SaveGenReport ("Stack high water mark: " + String(uxHighWaterMark));
    DEBUG_PRINT("Stack high water mark: " ); DEBUG_PRINTLN(String(uxHighWaterMark));

	uint32_t minFreeHeap = ESP.getMinFreeHeap();
	//Serial.println("Min free heap space: " + String(minFreeHeap));
	//SaveGenReport ("Min free heap space: " + String(minFreeHeap));
    DEBUG_PRINT("Min free heap space: "); DEBUG_PRINTLN(String(minFreeHeap));
}


// ─────────────────────────────────────────────────────────────────────────────
// TestOutPins  (development diagnostic — not called in normal operation)
// Sequentially activates each output pin for 2 seconds with a Serial message
// so the hardware wiring can be verified at initial commissioning.
// ─────────────────────────────────────────────────────────────────────────────
void TestOutPins()
{
	delay(500);
	DEBUG_PRINTLN(" "); DEBUG_PRINTLN(" "); DEBUG_PRINTLN("New"); DEBUG_PRINTLN(" ");
	for ( int i = 0 ; i < OutPorts; i++ )
	{
		digitalWrite(OutPort[i].Port, HIGH);
		DEBUG_PRINT("ON: "); DEBUG_PRINTLN(OutPort[i].Desc);
		delay(2000);
		digitalWrite(OutPort[i].Port, LOW);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// TestInPins  (development diagnostic — not called in normal operation)
// Maps each debounced digital input directly to its corresponding output so
// physical wiring of float switches and panel buttons can be verified at
// initial commissioning.
// ─────────────────────────────────────────────────────────────────────────────
void TestInPins()
{
	//
	if(InPort[I_TANK_FILTER_ON_OFF].Input)
	{
		digitalWrite(OutPort[O_TANK_FILTER_PUMP_RELAY].Port, HIGH);
		if(!OutPort[O_TANK_FILTER_PUMP_RELAY].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_TANK_FILTER_ON_OFF); Serial.print(" PinIO: "); Serial.println(InPort[I_TANK_FILTER_ON_OFF].PinIO);
//			Serial.println(OutPort[O_TANK_FILTER_PUMP_RELAY].Desc);
			OutPort[O_TANK_FILTER_PUMP_RELAY].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_TANK_FILTER_PUMP_RELAY].Port, LOW);
		OutPort[O_TANK_FILTER_PUMP_RELAY].OUTStat = false;
	}

	//
	if(InPort[I_SYSTEM_DISABLE_ENABLE].Input)
	{
		digitalWrite(OutPort[O_SYSTEM_DISA_ENA_LED].Port, HIGH);
		if(!OutPort[O_SYSTEM_DISA_ENA_LED].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_SYSTEM_DISABLE_ENABLE); Serial.print(" PinIO: "); Serial.println(InPort[I_SYSTEM_DISABLE_ENABLE].PinIO);
//			Serial.println(OutPort[O_SYSTEM_DISA_ENA_LED].Desc);
			OutPort[O_SYSTEM_DISA_ENA_LED].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_SYSTEM_DISA_ENA_LED].Port, LOW);
		OutPort[O_SYSTEM_DISA_ENA_LED].OUTStat = false;
	}

	//
	if(InPort[I_POOL_FILTER].Input)
	{
		digitalWrite(OutPort[O_POOL_PUMP_RELAY].Port, HIGH);
		if(!OutPort[O_POOL_PUMP_RELAY].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_POOL_FILTER); Serial.print(" PinIO: "); Serial.println(InPort[I_POOL_FILTER].PinIO);
//			Serial.println(OutPort[O_POOL_PUMP_RELAY].Desc);
			OutPort[O_POOL_PUMP_RELAY].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_POOL_PUMP_RELAY].Port, LOW);
		OutPort[O_POOL_PUMP_RELAY].OUTStat = false;
	}

	//
	if(InPort[I_POOL_LIGHT].Input)
	{
		digitalWrite(OutPort[O_POOL_LIGHT_RELAY].Port, HIGH);
		if(!OutPort[O_POOL_LIGHT_RELAY].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_POOL_LIGHT); Serial.print(" PinIO: "); Serial.println(InPort[I_POOL_LIGHT].PinIO);
//			Serial.println(OutPort[O_POOL_LIGHT_RELAY].Desc);
			OutPort[O_POOL_LIGHT_RELAY].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_POOL_LIGHT_RELAY].Port, LOW);
		OutPort[O_POOL_LIGHT_RELAY].OUTStat = false;
	}

	//
	if(InPort[I_PLUGS_ON_OFF].Input)
	{
		digitalWrite(OutPort[O_PLUGS_SWITCH_RELAY].Port, HIGH);
		if(!OutPort[O_PLUGS_SWITCH_RELAY].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_PLUGS_ON_OFF); Serial.print(" PinIO: "); Serial.println(InPort[I_PLUGS_ON_OFF].PinIO);
//			Serial.println(OutPort[O_PLUGS_SWITCH_RELAY].Desc);
			OutPort[O_PLUGS_SWITCH_RELAY].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_PLUGS_SWITCH_RELAY].Port, LOW);
		OutPort[O_PLUGS_SWITCH_RELAY].OUTStat = false;
	}

	//
	if(InPort[I_FLOAT_HIGH].Input)
	{
		digitalWrite(OutPort[O_TANK_HIGH_LED].Port, HIGH);
		if(!OutPort[O_TANK_HIGH_LED].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_FLOAT_HIGH ); Serial.print(" PinIO: "); Serial.println(InPort[I_FLOAT_HIGH ].PinIO);
//			Serial.println(OutPort[O_TANK_HIGH_LED].Desc);
			OutPort[O_TANK_HIGH_LED].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_TANK_HIGH_LED].Port, LOW);
		OutPort[O_TANK_HIGH_LED].OUTStat = false;
	}

	//
	if(InPort[I_FLOAT_LOW].Input)
	{
		digitalWrite(OutPort[O_TANK_LOW_LED].Port, HIGH);
		if(!OutPort[O_TANK_LOW_LED].OUTStat)
		{
//			Serial.print("IDX: ");  Serial.print(I_FLOAT_LOW ); Serial.print(" PinIO: "); Serial.println(InPort[I_FLOAT_LOW ].PinIO);
//			Serial.println(OutPort[O_TANK_LOW_LED].Desc);
			OutPort[O_TANK_LOW_LED].OUTStat = true;
		}
	}
	else
	{
		digitalWrite(OutPort[O_TANK_LOW_LED].Port, LOW);
		OutPort[O_TANK_LOW_LED].OUTStat = false;
	}

}

// ============================================================
// Include all module implementation files
// (mirrors how Arduino IDE concatenates .ino files)
// ============================================================
#include "01_Main.cpp"
#include "02_Time.cpp"
#include "03_Files.cpp"
#include "04_WebPages.cpp"
#include "05_ThisSystem.cpp"
#include "08_Home.cpp"
#include "09_Suspend.cpp"
#include "09_WaterValves.cpp"
#include "09_Irrigation.cpp"
#include "11_Other.cpp"
#include "12_AnalogRead.cpp"
#include "12_Tank.cpp"
#include "13_Pool.cpp"
#include "14_Plugs.cpp"
#include "12_Settings.cpp"
#include "15_About.cpp"
#include "16_NTFY.cpp"
#include "17_RainGauge.cpp"
#include "22_Update.cpp"
#include "05_WiFi.cpp"   // WiFi join + setup hotspot + /wifi page (needs `server`, so after the web modules)
