// =============================================================================
// 05_WiFi.cpp — WiFi join (saved / built-in networks) + setup hotspot
// =============================================================================
// See 05_WiFi.h for the order in which SetupWiFi() tries things.
// #included by main.cpp (single translation unit) — the setup hotspot uses the
// main AsyncWebServer `server`, so the old WebServer.h conflict is gone.
//
// /WiFiConfig.json  { "nets": [ { ssid, pass, dhcp, ip, gw, sn, dns }, ... ] }
//   Written by the setup hotspot / the /wifi page, or automatically when the
//   controller joins one of the KNOWN_NETS.  Holds WiFi passwords, so the
//   FILES pages never list, show or download it (IsProtectedFile, 03_Files.h).
//
// KNOWN_NETS come only from the private, git-ignored wifi_secrets.h
// (WIFI_KNOWN_NETS).  The public firmware has none: a new controller goes
// straight to the setup hotspot.
// =============================================================================

#include <ESPmDNS.h>
#include <DNSServer.h>
#include <climits>

// Optional private networks (wifi_secrets.h is NOT in Git).
#if __has_include("wifi_secrets.h")
	#include "wifi_secrets.h"
#endif

struct KnownNet {
	const char* ssid;
	const char* pass;
	uint8_t     ip[4];   // fixed IP on that network (http://<ip>/)
};

#ifdef WIFI_KNOWN_NETS
	static const KnownNet KNOWN_NETS[] = { WIFI_KNOWN_NETS };
	static const int KNOWN_NET_COUNT = sizeof(KNOWN_NETS) / sizeof(KNOWN_NETS[0]);
#else
	static const KnownNet* const KNOWN_NETS = nullptr;   // public firmware: setup hotspot only
	static const int KNOWN_NET_COUNT = 0;
#endif

#define WIFI_HOSTNAME        "hydro17"          // mDNS: http://hydro17.local/
#define WIFI_SETUP_SSID      "Hydro17-Setup"    // setup hotspot (open, 192.168.4.1)
#define WIFI_CONFIG_FILE     "/WiFiConfig.json"
#define WIFI_OLD_FILE        "/WifiSettings.json"   // old single-network file — imported once
#define WIFI_OLD_PROFILES    "/WifiProfiles.json"   // old profile list — deleted with it
#define WIFI_ROUNDS          6                  // full (saved + scan) rounds before the setup hotspot
#define WIFI_SAVED_TRIES     2                  // attempts per round when only ONE network is saved
#define WIFI_MAX_NETS        8                  // networks kept in /WiFiConfig.json
#define WIFI_CONNECT_MS      15000UL            // per join attempt
#define WIFI_ROUND_GAP_MS    10000UL            // pause between rounds (router may be rebooting)
#define WIFI_PORTAL_MS       (5UL * 60UL * 1000UL)  // setup hotspot after failed rounds: 5 min, then reboot

// The network as saved in /WiFiConfig.json
struct WifiCfg {
	String    ssid, pass;
	bool      dhcp = true;
	IPAddress ip, gw, sn, dns;
};

// ── Helpers ───────────────────────────────────────────────────────────────────

// Wait for WL_CONNECTED, blinking the PCB LED so "searching" is visible.
static bool wifiWaitConnected(){
	unsigned long t = millis();
	while (WiFi.status() != WL_CONNECTED) {
		if (millis() - t > WIFI_CONNECT_MS) {
			DEBUG_PRINTLN("WiFi: connection timed out");
			return false;
		}
		delay(100);
		digitalWrite(LEDOUT, !digitalRead(LEDOUT));
	}
	return true;
}

static void wifiDrop(){
	WiFi.disconnect(false);
	delay(300);
}

static bool wifiJoinWith(const WifiCfg& c){
	DEBUG_PRINTF("WiFi: joining %s (%s)\n", c.ssid.c_str(), c.dhcp ? "DHCP" : c.ip.toString().c_str());
	if (c.dhcp) WiFi.config(IPAddress(0,0,0,0), IPAddress(0,0,0,0), IPAddress(0,0,0,0));   // 0.0.0.0 = DHCP
	else        WiFi.config(c.ip, c.gw, c.sn, c.dns);
	// Several access points may share one SSID (mesh / multi-AP, Omada). The
	// ESP32's default fast scan joins the FIRST one found on the channel sweep;
	// scan all channels and pick the strongest instead.
	WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
	WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
	WiFi.begin(c.ssid.c_str(), c.pass.c_str());
	if (!wifiWaitConnected()) return false;
	SaveGenReport("WiFi: joined " + c.ssid + " via AP " + WiFi.BSSIDstr() +
	              " (channel " + String(WiFi.channel()) + ", " + String(WiFi.RSSI()) + " dBm)");
	return true;
}

// ── /WiFiConfig.json — a LIST of networks ────────────────────────────────────

static void wifiNetFromJson(JsonVariantConst o, WifiCfg& c){
	c.ssid = o["ssid"] | "";
	c.pass = o["pass"] | "";
	c.dhcp = o["dhcp"] | true;
	if (!c.dhcp){
		if (!c.ip.fromString(o["ip"] | "")){ c.dhcp = true; return; }   // no usable fixed IP: DHCP
		// gw / sn / dns may be missing = learned from the router at the first join.
		c.gw.fromString(o["gw"] | "");
		c.sn.fromString(o["sn"] | "");
		c.dns.fromString(o["dns"] | "");
	}
}

// Fixed IP without a known gateway: learn gateway / subnet / DNS by DHCP first.
static bool wifiNeedsLearning(const WifiCfg& c){
	return !c.dhcp && ((uint32_t)c.gw == 0 || (uint32_t)c.sn == 0);
}

// Fills out[] (max maxN), returns how many networks are saved.
static int wifiLoadNets(WifiCfg* out, int maxN){
	File fr = SPIFFS.open(WIFI_CONFIG_FILE, "r");
	if (!fr) return 0;
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, fr);
	fr.close();
	if (err) return 0;
	int n = 0;
	for (JsonVariantConst o : doc["nets"].as<JsonArrayConst>()){
		if (n >= maxN) break;
		wifiNetFromJson(o, out[n]);
		if (out[n].ssid.length()) n++;
	}
	return n;
}

static void wifiSaveNets(const WifiCfg* nets, int n){
	JsonDocument doc;
	JsonArray a = doc["nets"].to<JsonArray>();
	for (int i = 0; i < n; i++){
		JsonObject o = a.add<JsonObject>();
		o["ssid"] = nets[i].ssid;
		o["pass"] = nets[i].pass;
		o["dhcp"] = nets[i].dhcp;
		if (!nets[i].dhcp){
			o["ip"] = nets[i].ip.toString();
			if ((uint32_t)nets[i].gw)  o["gw"]  = nets[i].gw.toString();    // not yet learned = left out
			if ((uint32_t)nets[i].sn)  o["sn"]  = nets[i].sn.toString();
			if ((uint32_t)nets[i].dns) o["dns"] = nets[i].dns.toString();
		}
	}
	File fw = SPIFFS.open(WIFI_CONFIG_FILE, "w");
	if (!fw) { SaveGenReport("WiFi: could not write /WiFiConfig.json"); return; }
	serializeJson(doc, fw);
	fw.close();
}

// Add a network (newest first).  The same SSID again replaces its entry;
// with a full list the oldest entry drops off.
static void wifiAddNet(const WifiCfg& c){
	WifiCfg nets[WIFI_MAX_NETS + 1];
	int n = wifiLoadNets(nets + 1, WIFI_MAX_NETS);
	nets[0] = c;
	int k = 1;
	for (int i = 1; i <= n; i++)
		if (nets[i].ssid != c.ssid && k < WIFI_MAX_NETS) nets[k++] = nets[i];
	wifiSaveNets(nets, k);
}

// Remove a network by SSID.  Returns false when it was not in the list.
static bool wifiDeleteNet(const String& ssid){
	WifiCfg nets[WIFI_MAX_NETS];
	int n = wifiLoadNets(nets, WIFI_MAX_NETS), k = 0;
	for (int i = 0; i < n; i++) if (nets[i].ssid != ssid) nets[k++] = nets[i];
	if (k == n) return false;
	wifiSaveNets(nets, k);
	return true;
}

// Old firmware kept ONE network in /WifiSettings.json { ssid, pass, ip[4],
// gw[4], sn[4], dns[4] }.  Import it once into the list, then delete the old
// files, so an updated controller keeps its network and fixed IP.
static void wifiImportOld(){
	if (!SPIFFS.exists(WIFI_OLD_FILE)) return;
	File f = SPIFFS.open(WIFI_OLD_FILE, "r");
	JsonDocument doc;
	bool ok = f && !deserializeJson(doc, f);
	if (f) f.close();
	if (ok && (doc["ssid"] | "")[0]){
		WifiCfg c;
		c.ssid = doc["ssid"] | "";
		c.pass = doc["pass"] | "";
		JsonArrayConst ip = doc["ip"], gw = doc["gw"], sn = doc["sn"], dns = doc["dns"];
		if (ip.size() == 4 && gw.size() == 4 && sn.size() == 4 && dns.size() == 4 && (int)ip[0] != 0){
			c.dhcp = false;
			c.ip  = IPAddress((int)ip[0],  (int)ip[1],  (int)ip[2],  (int)ip[3]);
			c.gw  = IPAddress((int)gw[0],  (int)gw[1],  (int)gw[2],  (int)gw[3]);
			c.sn  = IPAddress((int)sn[0],  (int)sn[1],  (int)sn[2],  (int)sn[3]);
			c.dns = IPAddress((int)dns[0], (int)dns[1], (int)dns[2], (int)dns[3]);
		}
		WifiCfg nets[WIFI_MAX_NETS];
		int n = wifiLoadNets(nets, WIFI_MAX_NETS);
		bool have = false;
		for (int i = 0; i < n; i++) if (nets[i].ssid == c.ssid) have = true;
		if (!have){
			wifiAddNet(c);
			SaveGenReport("WiFi: old setting " + c.ssid + (c.dhcp ? String(" (DHCP)") : " on " + c.ip.toString()) +
			              " moved to the WiFi Networks list");
		}
	}
	SPIFFS.remove(WIFI_OLD_FILE);
	SPIFFS.remove(WIFI_OLD_PROFILES);
	SPIFFS.remove("/ProfileSwitch.txt");
}

// Write EVERY built-in network (KNOWN_NETS) into /WiFiConfig.json, not only
// the one joined: after "INSTALL FROM GITHUB" the controller runs the public
// firmware, which has no built-in networks, and then knows only this file —
// so a unit set up at one house must still find the other house's network.
// The gateway is left out and learned from the router at the first join
// there (wifiNeedsLearning).  An entry already in the file (same SSID) is
// kept as it is (it may hold a learned gateway or a user change).
static void wifiSeedKnown(){
	if (KNOWN_NET_COUNT == 0) return;
	WifiCfg nets[WIFI_MAX_NETS];
	int n = wifiLoadNets(nets, WIFI_MAX_NETS);
	for (int i = 0; i < KNOWN_NET_COUNT; i++){
		bool have = false;
		for (int j = 0; j < n; j++) if (nets[j].ssid == KNOWN_NETS[i].ssid) have = true;
		if (have || n >= WIFI_MAX_NETS) continue;
		WifiCfg c;
		c.ssid = KNOWN_NETS[i].ssid; c.pass = KNOWN_NETS[i].pass; c.dhcp = false;
		c.ip   = IPAddress(KNOWN_NETS[i].ip[0], KNOWN_NETS[i].ip[1], KNOWN_NETS[i].ip[2], KNOWN_NETS[i].ip[3]);
		nets[n++] = c;                     // appended: existing entries keep their order
		SaveGenReport("WiFi: built-in network " + c.ssid + " (" + c.ip.toString() + ") added to the saved networks");
	}
	wifiSaveNets(nets, n);
}

// ── Step 1: saved networks ───────────────────────────────────────────────────
// Scan, then try the saved networks that are in range, strongest first, and
// after them the ones not seen (hidden SSIDs, weak signal).  With a single
// saved network it gets WIFI_SAVED_TRIES attempts, otherwise one each.

static bool wifiJoinSaved(){
	WifiCfg nets[WIFI_MAX_NETS];
	int n = wifiLoadNets(nets, WIFI_MAX_NETS);
	if (n == 0) return false;

	int rssi[WIFI_MAX_NETS];
	for (int i = 0; i < n; i++) rssi[i] = INT_MIN;
	int found = WiFi.scanNetworks();
	for (int j = 0; j < found; j++)
		for (int i = 0; i < n; i++)
			if (WiFi.SSID(j) == nets[i].ssid && WiFi.RSSI(j) > rssi[i]) rssi[i] = WiFi.RSSI(j);
	WiFi.scanDelete();

	bool tried[WIFI_MAX_NETS] = { false };
	int tries = (n == 1) ? WIFI_SAVED_TRIES : 1;
	for (int k = 0; k < n; k++){
		int best = -1;                       // in range first (strongest), then the rest in list order
		for (int i = 0; i < n; i++)
			if (!tried[i] && (best < 0 || rssi[i] > rssi[best])) best = i;
		tried[best] = true;
		WifiCfg& c = nets[best];
		for (int t = 0; t < tries; t++){
			bool learned = false;
			if (wifiNeedsLearning(c)){
				// Join by DHCP once to learn the router's gateway / subnet / DNS.
				WifiCfg d = c; d.dhcp = true;
				if (!wifiJoinWith(d)){ wifiDrop(); continue; }
				c.gw = WiFi.gatewayIP();
				c.sn = WiFi.subnetMask();
				if ((uint32_t)c.dns == 0) c.dns = WiFi.dnsIP();
				wifiDrop();
				learned = true;
			}
			if (wifiJoinWith(c)){
				WiFiSource = "saved";
				if (learned){
					WifiCfg all[WIFI_MAX_NETS];          // store what was learned, same place in the list
					int m = wifiLoadNets(all, WIFI_MAX_NETS);
					for (int i = 0; i < m; i++) if (all[i].ssid == c.ssid) all[i] = c;
					wifiSaveNets(all, m);
					SaveGenReport("WiFi: " + c.ssid + " on " + c.ip.toString() + " - gateway " + c.gw.toString() +
					              " learned from the router and saved");
				}
				return true;
			}
			wifiDrop();
		}
	}
	return false;
}

// ── Step 2: known networks (private firmware only) ───────────────────────────
// Joined by DHCP first to LEARN the gateway / subnet / DNS, then rejoined on
// the fixed IP from the table and saved in /WiFiConfig.json.

static bool wifiScanAndJoinKnown(){
	if (KNOWN_NET_COUNT == 0) return false;
	DEBUG_PRINTLN("WiFi: scanning for known networks...");
	int n = WiFi.scanNetworks();

	int rssi[KNOWN_NET_COUNT + 1];
	for (int i = 0; i < KNOWN_NET_COUNT; i++) rssi[i] = INT_MIN;
	for (int j = 0; j < n; j++) {
		String found = WiFi.SSID(j);
		for (int i = 0; i < KNOWN_NET_COUNT; i++)
			if (found == KNOWN_NETS[i].ssid && WiFi.RSSI(j) > rssi[i]) rssi[i] = WiFi.RSSI(j);
	}
	WiFi.scanDelete();

	bool tried[KNOWN_NET_COUNT + 1] = { false };
	for (;;) {
		int best = -1;
		for (int i = 0; i < KNOWN_NET_COUNT; i++)
			if (!tried[i] && rssi[i] != INT_MIN && (best < 0 || rssi[i] > rssi[best])) best = i;
		if (best < 0) break;
		tried[best] = true;
		const KnownNet* k = &KNOWN_NETS[best];

		WifiCfg c;
		c.ssid = k->ssid; c.pass = k->pass; c.dhcp = true;
		if (!wifiJoinWith(c)) { wifiDrop(); continue; }
		c.dhcp = false;
		c.ip   = IPAddress(k->ip[0], k->ip[1], k->ip[2], k->ip[3]);
		c.gw   = WiFi.gatewayIP();
		c.sn   = WiFi.subnetMask();
		c.dns  = WiFi.dnsIP();
		wifiDrop();

		if (wifiJoinWith(c)) {
			wifiAddNet(c);
			WiFiSource = "built-in";
			SaveGenReport("WiFi: joined " + c.ssid + " on " + c.ip.toString() + " - added to the saved networks");
			return true;
		}
		wifiDrop();
	}
	return false;
}

// ── Setup hotspot ────────────────────────────────────────────────────────────
// Open hotspot "Hydro17-Setup" (192.168.4.1) with a captive page: pick the
// network, password, automatic (DHCP) or fixed IP.  The async_tcp handler
// only copies the form into pending + raises a flag; the loop below saves
// it and reboots.  Always ends in a reboot (save, cancel or timeout).

static WifiCfg       wifiPendingCfg;
static volatile bool wifiPendingSave   = false;
static volatile bool wifiPendingCancel = false;
static String        wifiPortalOptions;   // <option>s from the scan at start

static const char WIFI_PORTAL_1[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta name='viewport' content='width=device-width,initial-scale=1'><title>Hydro17 WiFi Setup</title>
<style>body{font-family:sans-serif;max-width:420px;margin:0 auto;padding:12px;background:#f8fafc;color:#0f172a}
h2{color:#0284c7;text-align:center;margin:6px 0}label{display:block;margin:10px 0 3px;font-weight:600;font-size:14px}
input,select{width:100%;box-sizing:border-box;padding:8px;font-size:15px;border:1px solid #cbd5e1;border-radius:5px}
.r{display:flex;gap:16px;margin-top:6px}.r label{font-weight:400;margin:0}.r input{width:auto}
button{width:100%;padding:12px;margin-top:16px;font-size:16px;font-weight:700;border:0;border-radius:6px;color:#fff;background:#16a34a}
.c{background:#64748b}p{font-size:13px;color:#475569}</style></head><body>
<h2>Hydro17 Water System &mdash; WiFi Setup</h2>
<p>Choose the WiFi network the controller must join. After SAVE it restarts and joins it.
Then open <b>http://hydro17.local/</b> (or the fixed IP you chose) on a phone or PC on that network.</p>
<form method='POST' action='/save'>
<label>WiFi network</label><select name='ssid'>)HTML";

static const char WIFI_PORTAL_2[] PROGMEM = R"HTML(<option value=''>Other (type below)...</option></select>
<label>...or type the network name</label><input name='ssid2' placeholder='only if not in the list'>
<label>WiFi password</label><input name='pass' type='password' placeholder='leave empty for an open network'>
<label>IP address</label>
<div class='r'><label><input type='radio' name='mode' value='dhcp' onclick="f(0)"> Automatic (DHCP)</label>
<label><input type='radio' name='mode' value='fixed' checked onclick="f(1)"> Fixed IP</label></div>
<div id='fx'>
<label>Fixed IP (e.g. 192.168.1.120)</label><input name='ip'>
<label>Gateway / router (empty = learned from the router)</label><input name='gw'>
<label>Subnet mask</label><input name='sn' value='255.255.255.0'>
<label>DNS (empty = same as gateway)</label><input name='dns'>
<p>A fixed IP is easiest: you always find the controller at the same address.</p></div>
<button type='submit'>SAVE &amp; RESTART</button></form>
<form method='POST' action='/cancel'><button class='c' type='submit'>CANCEL (keep the old setting)</button></form>
<script>function f(x){document.getElementById('fx').style.display=x?'block':'none';}</script>
</body></html>)HTML";

static void wifiPortalSend(AsyncWebServerRequest* req){
	String page;
	page.reserve(3800 + wifiPortalOptions.length());
	page += FPSTR(WIFI_PORTAL_1);
	page += wifiPortalOptions;
	page += FPSTR(WIFI_PORTAL_2);
	req->send(200, "text/html", page);
}

static String wifiArg(AsyncWebServerRequest* req, const char* name){
	String v = req->hasArg(name) ? req->arg(name) : String();
	v.trim();
	return v;
}

// Network from a form (setup hotspot page and the /wifi page "ADD NETWORK"):
// ssid (or ssid2 typed by hand), pass, mode = dhcp / fixed, ip, gw, sn, dns.
// Returns "" when fine, otherwise the reason.
static String wifiParseForm(AsyncWebServerRequest* req, WifiCfg& c){
	c.ssid = wifiArg(req, "ssid2");
	if (c.ssid.length() == 0) c.ssid = wifiArg(req, "ssid");
	c.pass = req->hasArg("pass") ? req->arg("pass") : String();
	c.dhcp = wifiArg(req, "mode") != "fixed";
	if (c.ssid.length() == 0 || c.ssid.length() > 32) return "Choose or type a WiFi network name (max 32 characters).";
	if (c.pass.length() > 63)                         return "The WiFi password can be at most 63 characters.";
	if (!c.dhcp){
		String gw = wifiArg(req, "gw"), sn = wifiArg(req, "sn"), dns = wifiArg(req, "dns");
		if (!c.ip.fromString(wifiArg(req, "ip"))) return "The fixed IP must be like 192.168.1.120.";
		if (gw.length() == 0){
			c.gw = c.sn = c.dns = IPAddress();   // learned from the router at the first join
			return "";
		}
		if (!c.gw.fromString(gw))                 return "The gateway must be like 192.168.1.1 (or leave it empty: learned from the router).";
		if (sn.length() == 0)                     c.sn = IPAddress(255, 255, 255, 0);
		else if (!c.sn.fromString(sn))            return "The subnet mask must be like 255.255.255.0.";
		if (dns.length() == 0)                    c.dns = c.gw;
		else if (!c.dns.fromString(dns))          return "DNS must be like 192.168.1.1 (or leave it empty).";
	}
	return "";
}

static void wifiPortalSave(AsyncWebServerRequest* req){
	WifiCfg c;
	String err = wifiParseForm(req, c);
	if (err.length()){
		req->send(200, "text/html", "<meta name='viewport' content='width=device-width,initial-scale=1'>"
		          "<p style='font-family:sans-serif;color:#b91c1c'>" + err + "</p><p><a href='/'>Back</a></p>");
		return;
	}
	wifiPendingCfg  = c;
	wifiPendingSave = true;
	req->send(200, "text/html", "<meta name='viewport' content='width=device-width,initial-scale=1'>"
	          "<div style='font-family:sans-serif;text-align:center;padding:20px'><h3>Saved</h3>"
	          "<p>The controller restarts and joins <b>" + c.ssid + "</b>.</p>"
	          "<p>Connect your phone back to your normal WiFi and open<br><b>" +
	          (c.dhcp ? String("http://hydro17.local/") : "http://" + c.ip.toString() + "/") + "</b></p>"
	          "<p style='color:#64748b;font-size:13px'>If it cannot join, the " WIFI_SETUP_SSID " hotspot comes back "
	          "after a few minutes so you can try again.</p></div>");
}

static void wifiRunSetupPortal(uint32_t timeoutMs, const String& why){
	SaveGenReport("WiFi: setup hotspot " WIFI_SETUP_SSID " started (" + why + ")");
	WiFi.disconnect(true);
	delay(200);
	WiFi.mode(WIFI_AP_STA);

	// Networks in range, strongest first, each SSID once.
	int n = WiFi.scanNetworks();
	{
		wifiPortalOptions = "";
		bool used[64] = { false };
		int m = n < 64 ? n : 64;
		for (int k = 0; k < m; k++){
			int best = -1;
			for (int j = 0; j < m; j++)
				if (!used[j] && (best < 0 || WiFi.RSSI(j) > WiFi.RSSI(best))) best = j;
			if (best < 0) break;
			used[best] = true;
			String s = WiFi.SSID(best);
			if (s.length() == 0 || s.indexOf('\'') >= 0 || s.indexOf('<') >= 0) continue;
			if (wifiPortalOptions.indexOf("value='" + s + "'") >= 0) continue;
			wifiPortalOptions += "<option value='" + s + "'>" + s + "  (" + String(WiFi.RSSI(best)) + " dBm)</option>";
		}
	}
	WiFi.scanDelete();

	WiFi.softAP(WIFI_SETUP_SSID);
	IPAddress apIP = WiFi.softAPIP();
	DNSServer dns;
	dns.start(53, "*", apIP);     // every name -> the controller: phones show the page by themselves

	server.on("/",       HTTP_GET,  wifiPortalSend);
	server.on("/save",   HTTP_POST, wifiPortalSave);
	server.on("/cancel", HTTP_POST, [](AsyncWebServerRequest* r){
		wifiPendingCancel = true;
		r->send(200, "text/html", "<p style='font-family:sans-serif'>Cancelled - the controller restarts with its old setting.</p>");
	});
	server.onNotFound([apIP](AsyncWebServerRequest* r){ r->redirect("http://" + apIP.toString() + "/"); });
	server.begin();
	DEBUG_PRINTLN("WiFi: setup hotspot " WIFI_SETUP_SSID " at " + apIP.toString());

	MsTick.attach_ms(150, ISR_LED_Setup);   // fast LED blink = setup mode

	uint32_t start = millis();
	for (;;){
		dns.processNextRequest();
		if (wifiPendingSave){
			delay(1500);                  // let the "Saved" page reach the phone
			wifiAddNet(wifiPendingCfg);
			SaveGenReport("WiFi: setup added network " + wifiPendingCfg.ssid +
			              (wifiPendingCfg.dhcp ? String(" (DHCP)") : " on " + wifiPendingCfg.ip.toString()));
			ESP.restart();
		}
		if (wifiPendingCancel){ delay(1000); ESP.restart(); }
		if (timeoutMs && millis() - start > timeoutMs){
			SaveGenReport("WiFi: setup hotspot timed out - restarting to try the saved networks again");
			ESP.restart();
		}
		delay(5);
	}
}

// ── Public API ────────────────────────────────────────────────────────────────

// Deletes the saved networks.  The built-in ones (private firmware) are
// re-added at the next start.
void ClearWifiSettings(){
	SPIFFS.remove(WIFI_CONFIG_FILE);
	SPIFFS.remove(WIFI_OLD_FILE);
	DEBUG_PRINTLN("WiFi: saved networks removed");
}

bool SetupWiFi() {
	WiFi.persistent(false);          // our own file, not the IDF NVS copy
	WiFi.setHostname(WIFI_HOSTNAME);
	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);

	wifiImportOld();                 // first start after the update: keep the old network
	wifiSeedKnown();                 // all built-in networks into the file (survive a public update)

	// CONTROLLER DIS/EN (GPIO 34) held HIGH or BOOT (GPIO 0) held LOW at
	// power-on -> setup hotspot (add / change a network).
	if (digitalRead(34)) {
		while (digitalRead(34)) { digitalWrite(LEDOUT, HIGH); delay(10); }   // wait for release
		wifiRunSetupPortal(WIFI_PORTAL_MS, "CONTROLLER button held at power-on");
	}
	if (digitalRead(ASPRESET) == LOW)
		wifiRunSetupPortal(WIFI_PORTAL_MS, "BOOT button held at power-on");

	// Nothing saved and no built-in networks (public firmware): set up.
	if (!SPIFFS.exists(WIFI_CONFIG_FILE) && KNOWN_NET_COUNT == 0)
		wifiRunSetupPortal(0, "no WiFi network saved yet");

	for (int round = 0; round < WIFI_ROUNDS; round++) {
		if (round > 0) {
			DEBUG_PRINTF("WiFi: round %d failed - retry in %lu s\n", round, WIFI_ROUND_GAP_MS / 1000);
			delay(WIFI_ROUND_GAP_MS);
		}
		bool joined = wifiJoinSaved();
		if (!joined) joined = wifiScanAndJoinKnown();

		if (joined) {
			digitalWrite(LEDOUT, HIGH);
			delay(300);
			digitalWrite(LEDOUT, LOW);
			DEBUG_PRINT("WiFi: connected to "); DEBUG_PRINT(WiFi.SSID());
			DEBUG_PRINT(" ("); DEBUG_PRINT(WiFiSource); DEBUG_PRINT("), IP = ");
			DEBUG_PRINTLN(WiFi.localIP().toString());
			if (MDNS.begin(WIFI_HOSTNAME)) {
				MDNS.addService("http", "tcp", 80);
				DEBUG_PRINTLN("WiFi: mDNS started as " WIFI_HOSTNAME ".local");
			}
			return true;
		}
	}
	// Nothing joined for several minutes: open the setup hotspot for a while
	// (maybe the password changed), then reboot and try again.
	wifiRunSetupPortal(WIFI_PORTAL_MS, "no saved network could be joined");
	return false;   // not reached — the hotspot always reboots
}

// =============================================================================
// /wifi page — "WiFi Networks": list (never the passwords), DELETE, ADD
// =============================================================================
// Built in async_tcp; it only READS the file.  ADD / DELETE (POST
// /wifinet/add, /wifinet/delete) copy the request into one pending slot and
// raise a flag; MaintainWifiNets() (loop) writes the file.  Changes are used
// at the next start.

static WifiCfg       wnPendingAdd;
static String        wnPendingDel;
static volatile bool wnAddReq = false, wnDelReq = false;

static String wifiHtmlEsc(const String& s){
	String o; o.reserve(s.length() + 8);
	for (size_t i = 0; i < s.length(); i++){
		char c = s[i];
		if      (c == '&')  o += "&amp;";
		else if (c == '<')  o += "&lt;";
		else if (c == '>')  o += "&gt;";
		else if (c == '"')  o += "&quot;";
		else if (c == 39)   o += "&#39;";
		else                o += c;
	}
	return o;
}

static const char WN_SECTION_TOP[] PROGMEM = R"HTML(
<div style='text-align:center;font-weight:800;letter-spacing:2px;color:#0284c7;font-size:15px;margin:16px 0 4px'>WIFI NETWORKS</div>
<p style='color:#475569;font-size:12px;padding:0 12px 4px'>At start-up the controller tries these networks, the strongest one in range first,
and joins the strongest access point of that network. If none can be reached for about 4 minutes it opens the hotspot <b>Hydro17-Setup</b>
(also: hold BOOT or the CONTROLLER button at power-on) &mdash; connect a phone to it and add a network there.
Changes are used at the next start. Passwords are never shown.</p>
<table style='width:100%;font-size:13px'>)HTML";

static const char WN_SECTION_ADD[] PROGMEM = R"HTML(</table>
<div style='margin:8px 12px;padding:8px;border:1px solid #cbd5e1;border-radius:6px;font-size:13px'>
<b>Add a network</b> (the same name again replaces that entry)<br>
<input id='wn_ssid' placeholder='WiFi network name' style='width:220px;margin:4px 0'>
<input id='wn_pass' type='password' placeholder='password (empty = open network)' style='width:220px;margin:4px 0'><br>
<label><input type='radio' name='wn_mode' id='wn_dhcp' onclick="wnFix(0)"> Automatic IP (DHCP)</label>
<label><input type='radio' name='wn_mode' id='wn_fixed' checked onclick="wnFix(1)"> Fixed IP</label>
<div id='wn_fx'>
<input id='wn_ip' placeholder='fixed IP, e.g. 192.168.1.120' style='width:220px;margin:2px 0'>
<input id='wn_gw' placeholder='gateway (empty = learned from the router)' style='width:220px;margin:2px 0'>
<input id='wn_sn' value='255.255.255.0' style='width:220px;margin:2px 0'>
<input id='wn_dns' placeholder='DNS (empty = gateway)' style='width:220px;margin:2px 0'></div>
<div style='margin-top:6px'><input type='button' value='ADD NETWORK' onclick='wnAdd()' style='font-size:14px;background-color:#166534;color:#fff;border:none;padding:6px 14px;border-radius:4px;cursor:pointer'></div>
<span id='wn_msg' style='font-size:12px'></span></div>
<script>
function wnFix(x){document.getElementById('wn_fx').style.display=x?'block':'none';}
function wnPost(u,b){var m=document.getElementById('wn_msg');
 fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b})
 .then(function(r){return r.text();}).then(function(t){m.textContent=t;if(t.indexOf('OK')==0)setTimeout(function(){location.reload();},1200);})
 .catch(function(){m.textContent='No answer from the controller';});}
function wnAdd(){var v=function(i){return encodeURIComponent(document.getElementById(i).value.trim());};
 var b='ssid='+v('wn_ssid')+'&pass='+encodeURIComponent(document.getElementById('wn_pass').value)+
 '&mode='+(document.getElementById('wn_fixed').checked?'fixed':'dhcp')+'&ip='+v('wn_ip')+'&gw='+v('wn_gw')+'&sn='+v('wn_sn')+'&dns='+v('wn_dns');
 wnPost('/wifinet/add',b);}
function wnDel(s,cur){if(!confirm('Delete the WiFi network '+s+'?'+(cur?' The controller is connected to it now: it stays connected until the next start, then tries the other networks.':'')))return;
 wnPost('/wifinet/delete','ssid='+encodeURIComponent(s));}
</script>
)HTML";

// GET /wifi — the WiFi Networks page.
void handleWifiNets(AsyncWebServerRequest *request){
	AsyncResponseStream *r = beginHtmlStream(request);
	if (!r) return;
	WifiCfg nets[WIFI_MAX_NETS];
	int n = wifiLoadNets(nets, WIFI_MAX_NETS);
	String cur = WiFi.SSID();

	r->print(webHeadPage);
	append_FileOption(FA_MESS_06, r);
	append_UL_OptionsMenu(PS_SETTINGS, r);
	r->print("<div style='font-size:13px;text-align:center;margin-top:10px'>Connected to <b>" + wifiHtmlEsc(cur) +
	         "</b> via AP " + WiFi.BSSIDstr() + " (channel " + String(WiFi.channel()) + ", " +
	         String(WiFi.RSSI()) + " dBm) &mdash; IP " + WiFi.localIP().toString() +
	         (WiFiSource.length() ? " (" + WiFiSource + ")" : String("")) + "</div>");
	r->print(FPSTR(WN_SECTION_TOP));
	if (n == 0) r->print("<tr><td colspan='3'>(no networks saved)</td></tr>");
	for (int i = 0; i < n; i++){
		String s  = wifiHtmlEsc(nets[i].ssid);
		bool   on = (nets[i].ssid == cur);
		r->print("<tr><td style='width:40%;padding:4px 12px'><b>" + s + "</b>" +
		         (on ? String(" <span style='color:#16a34a;font-size:11px'>(connected)</span>") : String("")) +
		         "</td><td style='font-size:12px;width:45%;text-align:left'>" +
		         (nets[i].dhcp ? String("automatic IP") : "fixed " + nets[i].ip.toString() +
		          ((uint32_t)nets[i].gw ? String("") : String(" (gateway learned at the first start)"))) +
		         "</td><td style='width:15%;white-space:nowrap'><button style='background:#dc2626;padding:2px 8px;font-size:11px' data-s='" + s +
		         "' onclick='wnDel(this.dataset.s," + (on ? "1" : "0") + ")'>DELETE</button></td></tr>");
	}
	r->print(FPSTR(WN_SECTION_ADD));
	r->print(webHTMLEnd);
	request->send(r);
}

void handleWifiNetAdd(AsyncWebServerRequest *request){
	if (wnAddReq || wnDelReq){ request->send(200, "text/plain", "Busy - try again"); return; }
	WifiCfg c;
	String err = wifiParseForm(request, c);
	if (err.length()){ request->send(200, "text/plain", err); return; }
	wnPendingAdd = c;
	wnAddReq = true;
	request->send(200, "text/plain", "OK - " + c.ssid + " added");
}

void handleWifiNetDelete(AsyncWebServerRequest *request){
	if (wnAddReq || wnDelReq){ request->send(200, "text/plain", "Busy - try again"); return; }
	String s = wifiArg(request, "ssid");
	if (s.length() == 0){ request->send(200, "text/plain", "No network given"); return; }
	wnPendingDel = s;
	wnDelReq = true;
	request->send(200, "text/plain", "OK - " + s + " deleted");
}

// Called every 10 ms from loop() (MaintainTimeCheck).
void MaintainWifiNets(){
	if (wnAddReq){
		wifiAddNet(wnPendingAdd);
		SaveGenReport("WiFi: network " + wnPendingAdd.ssid + " added on the WiFi page" +
		              (wnPendingAdd.dhcp ? String(" (automatic IP)") : " (fixed " + wnPendingAdd.ip.toString() + ")"));
		wnPendingAdd = WifiCfg();
		wnAddReq = false;
	}
	if (wnDelReq){
		if (wifiDeleteNet(wnPendingDel)) SaveGenReport("WiFi: network " + wnPendingDel + " deleted on the WiFi page");
		wnDelReq = false;
	}
}
