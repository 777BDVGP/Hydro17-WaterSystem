// =============================================================================
// 01_Main.cpp — Hardware Initialisation, Watchdog & LED Heartbeat
// =============================================================================
// Implements board-level startup (pin modes, I/O initialisation), the hardware
// watchdog ISR driven by a 1-second Ticker, the ISR_LED_Setup flash ISR used
// by 05_WiFi.cpp during captive-portal mode, and the periodic LED heartbeat.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// InitHardware
// Configures the on-board LED and the ASPRESET/LEDOUT pins, then calls
// SetupSystemIOs() to configure all application output and input pins.
// Called once from setup() before WiFi provisioning starts.
// ─────────────────────────────────────────────────────────────────────────────
void InitHardware(){
	// Build-in LED & Reset Input
	pinMode(LEDOUT, OUTPUT);
	digitalWrite(LEDOUT, HIGH);      // Turn LED on immediately as a power-on indicator
	pinMode(ASPRESET, INPUT_PULLUP);

	// Configure all application I/O pins for this system
	SetupSystemIOs();
}

// ─────────────────────────────────────────────────────────────────────────────
// WatchdogSetup
// Attaches the 1-second hardware Ticker that calls ISRwatchdog().
// Called at the end of setup(), just before loop() starts, so loop() can
// reset watchdogCount before the first ISR fires.
// ─────────────────────────────────────────────────────────────────────────────
void WatchdogSetup() {
  secondTick.attach(1, ISRwatchdog);
}

// ─────────────────────────────────────────────────────────────────────────────
// ISRwatchdog  (ISR — called every 1 second by secondTick Ticker)
// Increments watchdogCount.  If loop() (MaintainTimeCheck) has not reset the counter within
// Params->WatchdogTime seconds the firmware concludes a task has stalled and
// forces a reboot.
// ─────────────────────────────────────────────────────────────────────────────
void ISRwatchdog() {
	// ArduinoOTA.handle() blocks loop() for the whole transfer, so the counter
	// cannot be reset then — ArduinoOTA has its own timeouts during an upload.
	if (OTAInProgress) { watchdogCount = 0; return; }
	watchdogCount++;
	if ( watchdogCount > Params->WatchdogTime ) // Seconds without a reset = task stall
	{
		SaveGenReport F("REBOOT - Watchdog");
		delay(100);
		ESP.restart();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// LEDToggle  (called every 10 ms from MaintainTimeCheck)
// Performs two duties on each call:
//   1. Monitors the ASPRESET button; hold for TIM_ASP_RESET (5 s) to clear
//      saved WiFi credentials and reboot into captive-portal AP mode.
//   2. Toggles the activity LED at TIM_LED_FLASHING_SPEED (600 ms) as a
//      visible heartbeat confirming the firmware is running.
// ─────────────────────────────────────────────────────────────────────────────
void LEDToggle(){ // From MaintainTimeCheck
	// ── WiFi Credential Reset ─────────────────────────────────────────────────
	// Hold ASPRESET LOW for 5 seconds to erase /WifiSettings.json and reboot
	// into the captive-portal AP for reconfiguration.
	if (!digitalRead(ASPRESET)){    // Hold for TIM_ASP_RESET (5 s)
		if(TimerExpired(TIM_ASP_RESET)){
			delay(500);
			ClearAboutIP();       // Remove ip/gw/sn/dns from About.json display
			ClearWifiSettings();  // Erase /WifiSettings.json → portal on next boot
			SaveGenReport F("REBOOT - Button press ASP reset");
			delay(500);
			ESP.restart();
		}
	}
	else{
		activateTimer(TIM_ASP_RESET); // Restart 5-second hold timer on button release
	}

	// ── LED Heartbeat ──────────────────────────────────────────────────────────
	// Toggle the activity LED every TIM_LED_FLASHING_SPEED (600 ms) to give a
	// visible indication that the firmware is running and tasks are alive.
	if(TimerExpired(TIM_LED_FLASHING_SPEED)){
		activateTimer(TIM_LED_FLASHING_SPEED);
		digitalWrite(LEDOUT, !digitalRead(LEDOUT));
	}
}

// ─── Gateway Keepalive State ──────────────────────────────────────────────────
// Written from the IDF ping task / WiFi event task, read from loop().
static esp_ping_handle_t  gwPing             = nullptr;
static volatile uint32_t  gwLastReplyMs      = 0;
static volatile uint32_t  gwMissed           = 0;      // Consecutive ping timeouts
static volatile uint32_t  wifiDropCount      = 0;      // STA_DISCONNECTED events seen
static volatile uint8_t   wifiDropReason     = 0;      // wifi_err_reason_t of the last one
static volatile bool      gwRestartRequested = false;  // Set by GatewayWatchRestart()

static void gwPingReply(esp_ping_handle_t, void*)   { gwLastReplyMs = millis(); gwMissed = 0; }
static void gwPingTimeout(esp_ping_handle_t, void*) { gwMissed++; }

// ─────────────────────────────────────────────────────────────────────────────
// GatewayWatchRestart  (called from UpdateSettingsPageValues, async_tcp task)
// Asks MaintainGatewayWatch() to tear down the ping session so a changed
// enable flag takes effect without a reboot.
// ─────────────────────────────────────────────────────────────────────────────
void GatewayWatchRestart(){
	gwRestartRequested = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MaintainGatewayWatch  (called every 10 ms from MaintainTimeCheck)
// KEEPALIVE + REBOOT SAFETY NET — ported from Prima3_pomphuis. Do NOT remove
// this as "just diagnostics".
//
// On the TP-Link Omada networks this unit runs on, a mostly passive ESP gets
// aged out of the AP/switch/router tables: it then answers neither ping nor
// ARP while WiFi.status() still says WL_CONNECTED and all tasks keep running,
// so neither the software watchdog nor the WiFi-lost guard in MaintainTimeCheck
// ever fires. Pinging the router every GW_PING_INTERVAL_MS (non-blocking, from
// the IDF ping task) keeps those tables fresh; if no reply arrives for
// GW_UNREACHABLE_MS the unit logs and reboots.
//
// Target: always the gateway (router) of the network the unit is on, so it
// is correct at either house. Enabled by Params->KeepAliveEnabled (Settings).
// Also logs every WiFi disconnect event with its reason code, so the
// GeneralLog shows what the link was doing before a reboot.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainGatewayWatch(){
	static bool     eventHooked = false;
	static uint32_t loggedDrops = 0;

	if (!eventHooked){
		WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info){
			wifiDropReason = info.wifi_sta_disconnected.reason;
			wifiDropCount++;
		}, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
		eventHooked = true;
	}

	if (wifiDropCount != loggedDrops){
		loggedDrops = wifiDropCount;
		SaveGenReport("WiFi disconnected, reason " + String(wifiDropReason) +
		              " (drops since boot: " + String(loggedDrops) + ")");
	}

	// ── Settings changed: drop the session, recreate below if still enabled ──
	if (gwRestartRequested){
		gwRestartRequested = false;
		if (gwPing){
			esp_ping_stop(gwPing);
			esp_ping_delete_session(gwPing);
			gwPing = nullptr;
			SaveGenReport F("Keepalive ping stopped (settings changed)");
		}
	}

	if (!Params->KeepAliveEnabled) return;

	// ── Start the ping session once WiFi is up ────────────────────────────────
	if (!gwPing){
		if (WiFi.status() != WL_CONNECTED) return;

		IPAddress target = WiFi.gatewayIP();

		esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
		cfg.target_addr.type            = IPADDR_TYPE_V4;
		cfg.target_addr.u_addr.ip4.addr = static_cast<uint32_t>(target);
		cfg.count                       = ESP_PING_COUNT_INFINITE;
		cfg.interval_ms                 = GW_PING_INTERVAL_MS;
		cfg.timeout_ms                  = GW_PING_TIMEOUT_MS;

		esp_ping_callbacks_t cbs = {};
		cbs.on_ping_success = gwPingReply;
		cbs.on_ping_timeout = gwPingTimeout;

		gwLastReplyMs = millis();
		gwMissed      = 0;
		if (esp_ping_new_session(&cfg, &cbs, &gwPing) != ESP_OK){
			gwPing = nullptr;
			return;
		}
		esp_ping_start(gwPing);
		SaveGenReport("Keepalive ping started -> " + target.toString());
		return;
	}

	// OTA holds the link busy and legitimately delays replies — never reboot then.
	if (OTAInProgress){ gwLastReplyMs = millis(); return; }

	// Read the reply stamp before millis() so a reply landing in between can
	// never make the difference wrap negative.
	uint32_t lastReply = gwLastReplyMs;
	if (millis() - lastReply > GW_UNREACHABLE_MS){
		SaveGenReport("REBOOT - Gateway unreachable " + String(GW_UNREACHABLE_MS / 60000UL) +
		              " min (missed " + String(gwMissed) + ", status " + String(WiFi.status()) +
		              ", RSSI " + String(WiFi.RSSI()) + ", heap " + String(ESP.getFreeHeap()) +
		              ", min heap " + String(ESP.getMinFreeHeap()) + ")");
		delay(500);
		ESP.restart();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// ISR_LED_Setup  (ISR — called every 150 ms by MsTick during captive portal)
// Flashes the activity LED rapidly to show the board is in AP/portal mode.
// ─────────────────────────────────────────────────────────────────────────────
void ISR_LED_Setup() {
	digitalWrite(LEDOUT, !digitalRead(LEDOUT));
}




// ─── NTFY.sh Push Notifications ───────────────────────────────────────────────
// NtfyTask, MessageNTFY, MaintainNTFY, and all related code live in 16_NTFY.cpp.
