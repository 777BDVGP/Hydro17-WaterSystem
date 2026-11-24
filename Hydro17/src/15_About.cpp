// =============================================================================
// 15_About.cpp — About Page Handler & OTA Setup
// =============================================================================
// Implements:
//   SetupOTA()       — configures ArduinoOTA with event callbacks that
//                      broadcast live progress to connected WebSocket clients
//   handleAbout()    — serves the About page showing OTA info, build date,
//                      and system information
//   handleSetASP()   — deletes /WifiSettings.json and reboots into
//                      captive-portal AP mode
//   handleSetReboot()— performs a clean soft reboot
//
// Build date is read from the compiler __DATE__/__TIME__ macros.
// =============================================================================

// ─── OTA State ────────────────────────────────────────────────────────────────
bool OTAInProgress      = false;
int  OTAPercentComplete = 0;

// =============================================================================
// About.json — IP Persistence
// =============================================================================

// -----------------------------------------------------------------------------
// ClearAboutIP
// Removes the ip, gw, sn, and dns fields from /About.json while preserving
// all other fields.  Called before ClearWifiSettings() so that stale IP data
// does not survive a portal reset.
// -----------------------------------------------------------------------------
void ClearAboutIP() {
	JsonDocument doc;
	File fr = SPIFFS.open(ABOUT_FILE_PATH, FILE_READ);
	if (fr) { deserializeJson(doc, fr); fr.close(); }
	doc.remove("ip");
	doc.remove("gw");
	doc.remove("sn");
	doc.remove("dns");
	File fw = SPIFFS.open(ABOUT_FILE_PATH, FILE_WRITE);
	if (fw) { serializeJson(doc, fw); fw.close(); }
	DEBUG_PRINTLN("About.json IP cleared");
}

// -----------------------------------------------------------------------------
// SaveAboutIP
// Merges the four IP values (ip, gw, sn, dns) into /About.json, preserving
// any existing fields.  Called from setup() after WiFi connects so that
// About.json always reflects the address the device is actually using.
// -----------------------------------------------------------------------------
void SaveAboutIP(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns) {
	JsonDocument doc;
	File fr = SPIFFS.open(ABOUT_FILE_PATH, FILE_READ);
	if (fr) { deserializeJson(doc, fr); fr.close(); }
	doc["ip"]  = ip.toString();
	doc["gw"]  = gw.toString();
	doc["sn"]  = sn.toString();
	doc["dns"] = dns.toString();
	File fw = SPIFFS.open(ABOUT_FILE_PATH, FILE_WRITE);
	if (!fw) { DEBUG_PRINTLN("Failed to write About.json"); return; }
	serializeJson(doc, fw);
	fw.close();
	DEBUG_PRINTLN("About.json IP saved: " + ip.toString());
}

// =============================================================================
// Build Date Helper
// =============================================================================

// -----------------------------------------------------------------------------
// GetBuildDate
// Reformats the compiler's __DATE__ ("Apr  3 2026") and __TIME__ ("18:06:53")
// macros into the human-readable string "03 Apr 2026 18:06:53".
// Single-digit day values are zero-padded.
// -----------------------------------------------------------------------------
static String GetBuildDate() {
	const char* d = __DATE__;
	const char* t = __TIME__;
	char mon[4] = { d[0], d[1], d[2], 0 };
	char day[3] = { d[4], d[5], 0 };
	char yr[5]  = { d[7], d[8], d[9], d[10], 0 };
	String dayStr = String(day);
	dayStr.trim();
	if (dayStr.length() == 1) dayStr = "0" + dayStr;
	return dayStr + " " + String(mon) + " " + String(yr) + " " + String(t);
}

// =============================================================================
// Page Fragment Builders
// =============================================================================

// -----------------------------------------------------------------------------
// webResetASPButton
// Returns an HTML button that navigates to /setASP — deletes /WifiSettings.json
// and reboots into captive-portal AP mode for WiFi reconfiguration.
// -----------------------------------------------------------------------------
static void webResetASPButton(AsyncResponseStream* r) {
	r->print(F("<div><br></div><div><b><a>"
	           "<b><a><input type=\"button\" id=\"RESETASP\" onclick=\"window.location.href='/setASP'\" "
	           "style=\"font-size:14px;background-color:#dc2626;color:#fff;"
	           "border:none;padding:6px 14px;border-radius:4px;cursor:pointer\" "
	           "value=\"RESET ASP\"</a></b></div>"));
}

// -----------------------------------------------------------------------------
// buildAboutTable
// Builds the OTA parameter table using the standard WT_* helpers.
// Shows the OTA status (live via WebSocket) and the PlatformIO upload command.
// -----------------------------------------------------------------------------
static void buildAboutTable(AsyncResponseStream* response) {
	int ID = ID_ABOUT;
	String otaCmd = "pio run -t upload --upload-port " + WiFi.localIP().toString();

	resetPageBuf();
	WT_StartPage();
	WT_Header("PARAMETER"); WT_Header("VALUE");

	// Row 1: OTA status (updated live via WebSocket setOtaStatus)
	{ char idBuf[8]; snprintf(idBuf, sizeof(idBuf), "%d", ID);
	  WT_StartLine();
	  WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text", "OTA (Over-The-Air) Updates", idBuf, "", nullptr, 0);
	  WT_Column(WT_INPUT, WT_FIRST, WT_LAST, "text", "Ready", "ota_status", COLOR_GREEN.c_str(), nullptr, "{n:\"readonly\",v:\"\"},");
	  WT_EndLine(); }
	ID++;

	// Row 2: PlatformIO terminal upload command (two labels + empty input)
	{ char idBuf[8]; snprintf(idBuf, sizeof(idBuf), "%d", ID);
	  WT_StartLine();
	  WT_Column(WT_LABEL, WT_FIRST, WT_MORE, "text", "OTA Terminal Command", idBuf, "", nullptr, 0);
	  WT_Column(WT_LABEL, WT_NEXT,  WT_LAST, "text", otaCmd.c_str(), idBuf, "font-size:11px", nullptr, 0);
	  WT_Column(WT_INPUT, WT_FIRST, WT_LAST, "", "", "", "", nullptr, 0);
	  WT_EndLine(); }
	ID++;

	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

// -----------------------------------------------------------------------------
// buildProjectInfoDiv
// Returns a read-only HTML block showing the project name, build date,
// and PCB revision.
// -----------------------------------------------------------------------------
static void buildProjectInfoDiv(AsyncResponseStream* r) {
	// Same look as the FIRMWARE UPDATE block (appendFwUpdateBlock, 22_Update.cpp).
	r->print(F("<div style='text-align:center;font-weight:800;letter-spacing:2px;color:#0284c7;font-size:15px;margin:14px 0 4px'>PROJECT DETAILS</div>"
	           "<table><tr><td>Project</td><td>Hydro17 Water System</td></tr>"
	           "<tr><td>Firmware</td><td>" FW_VERSION "</td></tr>"
	           "<tr><td>Build Date</td><td>"));
	r->print(GetBuildDate());
	r->print(F("</td></tr>"
	           "<tr><td>PCB</td><td>WEMOS_ESP32_2S_V5a</td></tr>"
	           "<tr><td>Board</td><td>WEMOS LOLIN S2 Mini</td></tr></table>"));
}

// -----------------------------------------------------------------------------
// buildNetworkDiv
// NETWORK block (like the Rain Gauge ABOUT page): WiFi network + access point,
// signal, IP, gateway / subnet, DNS, mDNS name, MAC.  Snapshot at page load.
// -----------------------------------------------------------------------------
static void buildNetworkDiv(AsyncResponseStream* r) {
	r->print(F("<div style='text-align:center;font-weight:800;letter-spacing:2px;color:#0284c7;font-size:15px;margin:14px 0 4px'>NETWORK</div><table>"));
	r->print("<tr><td>WiFi Network</td><td>" + WiFi.SSID() + (WiFiSource.length() ? " (" + WiFiSource + ")" : String("")) + "</td></tr>");
	r->print("<tr><td>Access Point</td><td>" + WiFi.BSSIDstr() + " (channel " + String(WiFi.channel()) + ")</td></tr>");
	r->print("<tr><td>Signal (dBm)</td><td>" + String(WiFi.RSSI()) + "</td></tr>");
	r->print("<tr><td>IP Address</td><td>" + WiFi.localIP().toString() + "</td></tr>");
	r->print("<tr><td>Gateway / Subnet</td><td>" + WiFi.gatewayIP().toString() + " / " + WiFi.subnetMask().toString() + "</td></tr>");
	r->print("<tr><td>DNS</td><td>" + WiFi.dnsIP().toString() + "</td></tr>");
	r->print(F("<tr><td>mDNS</td><td>http://hydro17.local/</td></tr>"));
	r->print("<tr><td>MAC Address</td><td>" + WiFi.macAddress() + "</td></tr></table>");
}

// =============================================================================
// Route Handlers
// =============================================================================

// -----------------------------------------------------------------------------
// handleAbout  (GET /about)
// Serves the About page with OTA info, live status, and project details.
// -----------------------------------------------------------------------------
void handleAbout(AsyncWebServerRequest *request) {
	//SaveGenReport("handleAbout " + String(ESP.getFreeHeap()));
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_OptionsMenu(PS_ABOUT, response);
	response->print(webScriptTag);
	// PC upload (ArduinoOTA) rows: only with SETTINGS "Show PC Upload (OTA) on
	// ABOUT".  ArduinoOTA itself stays on either way — hiding is cosmetic.
	if (Params->ShowOtaInfo) buildAboutTable(response);
	buildNetworkDiv(response);        // NETWORK (like the Rain Gauge)
	appendFwUpdateBlock(response);    // FIRMWARE UPDATE (22_Update.cpp)
	webRebootButton(response);
	webResetASPButton(response);
	buildProjectInfoDiv(response);
	response->print(webHTMLEnd);
	request->send(response);
}

// -----------------------------------------------------------------------------
// handleSetASP  (GET /setASP)
// Deletes /WifiSettings.json and reboots into the captive-portal AP so the
// user can enter new WiFi credentials.
// -----------------------------------------------------------------------------
void handleSetASP(AsyncWebServerRequest *request) {
	//SaveGenReport("handleSetASP " + String(ESP.getFreeHeap()));
	sendHtmlPage(request, "<html><body><h3>Saved WiFi networks removed. Rebooting...</h3></body></html>");
	delay(200);
	ClearAboutIP();       // Remove ip/gw/sn/dns from About.json display
	ClearWifiSettings();  // Delete /WifiSettings.json → portal on next boot
	SaveGenReport("REBOOT - WiFi settings cleared via web (setASP)");
	delay(100);
	ESP.restart();
}

// -----------------------------------------------------------------------------
// handleSetReboot  (GET /setReboot)
// Performs a clean soft reboot.
// -----------------------------------------------------------------------------
void handleSetReboot(AsyncWebServerRequest *request) {
	//SaveGenReport("handleSetReboot " + String(ESP.getFreeHeap()));
	sendHtmlPage(request, "<html><body><h3>Rebooting...</h3></body></html>");
	SaveGenReport("REBOOT - Manual reboot via web page");
	delay(200);
	ESP.restart();
}

// =============================================================================
// OTA Setup
// =============================================================================

// -----------------------------------------------------------------------------
// SetupOTA
// Configures ArduinoOTA with the system name as the mDNS hostname and
// registers four event callbacks:
//   onStart    — sets OTAInProgress; broadcasts "ota_start" to WebSocket
//   onEnd      — clears OTAInProgress; broadcasts "ota_end"
//   onProgress — updates OTAPercentComplete; broadcasts "ota_progress" every 10 %
//   onError    — clears OTAInProgress; broadcasts "ota_error"
// Then calls ArduinoOTA.begin() to start listening on port 3232.
// Called once from setup() after WiFi connects.
// -----------------------------------------------------------------------------
void SetupOTA() {
	String deviceName = Params->SysName;
	deviceName.trim();
	if (deviceName.length() == 0) deviceName = "WaterSystem";

	ArduinoOTA.setPort(3232);
	ArduinoOTA.setHostname(deviceName.c_str());

	ArduinoOTA
		.onStart([]() {
			OTAInProgress = true;
			OTAPercentComplete = 0;
			secondTick.detach();  // disable watchdog — flash writes can block >5 s
			ws.textAll("{\"type\":\"ota_start\"}");
			DEBUG_PRINTLN("OTA Start");
		})
		.onEnd([]() {
			DEBUG_PRINTLN("\nOTA End");
			ws.textAll("{\"type\":\"ota_end\"}");
			OTAInProgress = false;
			secondTick.attach(1, ISRwatchdog);  // re-enable watchdog
		})
		.onProgress([](unsigned int progress, unsigned int total) {
			int pct = (progress / (total / 100));
			if (pct != OTAPercentComplete && pct % 5 == 0) {
				String msg = "{\"type\":\"ota_progress\",\"percent\":" + String(pct) + "}";
				ws.textAll(msg);
			}
			OTAPercentComplete = pct;
			DEBUG_PRINTF("OTA Progress: %u%%\r", OTAPercentComplete);
		})
		.onError([](ota_error_t error) {
			DEBUG_PRINTF("OTA Error[%u]\n", error);
			ws.textAll("{\"type\":\"ota_error\"}");
			OTAInProgress = false;
			secondTick.attach(1, ISRwatchdog);  // re-enable watchdog
		});

	ArduinoOTA.begin();
	DEBUG_PRINTLN("OTA Ready - " + WiFi.localIP().toString());
	DEBUG_PRINTLN("PIO upload: pio run -t upload --upload-port " + WiFi.localIP().toString());
}
