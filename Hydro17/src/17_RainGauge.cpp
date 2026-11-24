// =============================================================================
// 17_RainGauge.cpp — RAIN_OK_DO_IRRI from the RainGauge_PIO unit
// =============================================================================
// See 17_RainGauge.h.
// =============================================================================

// =============================================================================
// RainGaugeTask — permanent FreeRTOS task (Core 0, priority 1)
// Blocks on RainGaugeReqQueue; for each request GETs <url>api/irrigate and
// posts the parsed result to RainGaugeResQueue.  No SPIFFS, no globals.
// =============================================================================
void RainGaugeTask(void *pvParameters)
{
	RainGaugeReq req;
	for (;;)
	{
		if (xQueueReceive(RainGaugeReqQueue, &req, portMAX_DELAY) != pdTRUE) continue;

		RainGaugeRes res;
		memset(&res, 0, sizeof(res));
		res.irrigate = true;

		WiFiClient client;
		HTTPClient http;
		http.setConnectTimeout(RG_HTTP_TIMEOUT);
		http.setTimeout(RG_HTTP_TIMEOUT);
		if (http.begin(client, String(req.url) + "api/irrigate"))
		{
			res.httpCode = http.GET();
			if (res.httpCode == 200)
			{
				JsonDocument doc;
				if (!deserializeJson(doc, http.getString()) && doc["irrigate"].is<bool>())
				{
					res.ok       = true;
					res.irrigate = doc["irrigate"].as<bool>();
					res.sim      = doc["sim"] | false;
					strlcpy(res.reason, doc["reason"] | "", sizeof(res.reason));
				}
				else
				{
					strlcpy(res.reason, "bad reply (not a rain gauge answer)", sizeof(res.reason));
				}
			}
			http.end();
		}
		xQueueOverwrite(RainGaugeResQueue, &res);
	}
}

// SetupRainGauge — creates the queues and launches RainGaugeTask.
// Called from setup() after WiFi connects.
void SetupRainGauge() {
	// Only started while "Use Rain Gauge" is ticked: the 6 KB stack costs nothing
	// while it is off (internal RAM is tight on the S2 with the new core).
	if (!Params->RainGaugeEnable || RainGaugeTaskHandle != NULL) return;
	RainGaugeReqQueue = xQueueCreate(1, sizeof(RainGaugeReq));
	RainGaugeResQueue = xQueueCreate(1, sizeof(RainGaugeRes));
	BaseType_t res = xTaskCreatePinnedToCore(RainGaugeTask, "RainGaugeTask", 6144, NULL, 1, &RainGaugeTaskHandle, 0);
	if (res == pdPASS) DEBUG_PRINTLN(F("RainGaugeTask created OK"));
	else               DEBUG_PRINTLN(F("RainGaugeTask creation FAILED"));
}

// "192.168.1.124" / "http://192.168.1.124" -> "http://192.168.1.124/"
static String rgNormaliseUrl(String url){
	url.trim();
	if (url.length() == 0) return url;
	if (!url.startsWith("http://")) url = "http://" + url;
	if (!url.endsWith("/"))         url += "/";
	return url;
}

// =============================================================================
// MaintainRainGauge  (called every 10 ms from loop())
// Owns RAIN_OK_DO_IRRI: true when the feature is off; otherwise the gauge's
// latest answer (with the stale fall-back described in 17_RainGauge.h).
// =============================================================================
void MaintainRainGauge()
{
	static uint32_t lastPollMs = 0;
	static uint32_t lastGoodMs = 0;     // 0 = no good answer yet
	static bool     lastOk     = true;  // for logging only the change ok <-> failed
	static int      lastLogged = -1;    // last logged answer (-1 none, 0 skip, 1 irrigate)

	// Badge on the Settings page: green = may irrigate, red = skip today.
	if (!Params->RainGaugeEnable)  RainGaugeBadge = "IRRIGATE  (rain gauge off)";
	else if (RAIN_OK_DO_IRRI)      RainGaugeBadge = "IRRIGATE";
	else                           RainGaugeBadge = "NO IRRIGATION TODAY";
	RainGaugeBadgeStyle = RAIN_OK_DO_IRRI || !Params->RainGaugeEnable ? RG_BADGE_GREEN : RG_BADGE_RED;

	if (Params->RainGaugeEnable && RainGaugeTaskHandle == NULL) SetupRainGauge();   // ticked after boot
	if (!Params->RainGaugeEnable || RainGaugeReqQueue == NULL)
	{
		RAIN_OK_DO_IRRI = true;
		RainGaugeStatus = "Not active - irrigating as normal";
		lastPollMs = lastGoodMs = 0;
		lastOk     = true;
		lastLogged = -1;
		return;
	}

	// ── Request a poll: at once after enable / Settings save, then hourly.
	if (RainGaugePollNow || lastPollMs == 0 || millis() - lastPollMs >= RG_POLL_MS)
	{
		String url = rgNormaliseUrl(Params->RainGaugeURL);
		if (url.length() > 0 && WiFi.status() == WL_CONNECTED)
		{
			RainGaugeReq req;
			strlcpy(req.url, url.c_str(), sizeof(req.url));
			xQueueOverwrite(RainGaugeReqQueue, &req);
			if (lastGoodMs == 0) RainGaugeStatus = "Asking " + url + " ...";
		}
		else if (url.length() == 0)
		{
			RainGaugeStatus = "Rain Gauge IP is empty - irrigating as normal";
			RAIN_OK_DO_IRRI = true;
		}
		RainGaugePollNow = false;
		lastPollMs = millis() | 1;
	}

	// ── Apply a result from the task.
	RainGaugeRes res;
	if (xQueueReceive(RainGaugeResQueue, &res, 0) != pdTRUE) return;

	String at = GetCurrentTime();
	if (res.ok)
	{
		RAIN_OK_DO_IRRI = res.irrigate;
		lastGoodMs = millis();
		RainGaugeStatus = String(res.irrigate ? "IRRIGATE" : "SKIP TODAY") + " - " + res.reason +
		                  (res.sim ? "  [GAUGE IN SIMULATION]" : "") + "  (read " + at + ")";
		if (!lastOk) SaveGenReport("Rain gauge reachable again");
		if ((int)res.irrigate != lastLogged)
		{
			SaveGenReport(String("Rain gauge: ") + (res.irrigate ? "irrigate" : "skip irrigation") + " - " + res.reason);
			lastLogged = res.irrigate;
		}
		lastOk = true;
		return;
	}

	// ── Failed: keep the last good answer for a while, then fail safe.
	String why = res.reason[0] ? String(res.reason) : ("HTTP " + String(res.httpCode));
	if (lastGoodMs != 0 && millis() - lastGoodMs < RG_STALE_MS)
	{
		RainGaugeStatus = "Not reachable (" + why + ") at " + at + " - keeping last answer: " +
		                  (RAIN_OK_DO_IRRI ? "IRRIGATE" : "SKIP");
	}
	else
	{
		RAIN_OK_DO_IRRI = true;
		RainGaugeStatus = "Not reachable (" + why + ") at " + at + " - irrigating as normal";
	}
	if (lastOk) SaveGenReport("Rain gauge not reachable (" + why + ")");
	lastOk = false;
}

// SendRainGaugeValues — push the badge / status on the Settings page when they
// change (called from MaintainSettingsPageValues, 12_Settings.cpp).
void SendRainGaugeValues(){
	static String lastS, lastB, lastBS;
	if (RainGaugeStatus != lastS){ webParamUpdate(RainGaugeStatus, "RGSTAT", "", "style"); lastS = RainGaugeStatus; }
	if (RainGaugeBadge != lastB || RainGaugeBadgeStyle != lastBS){
		webParamUpdate(RainGaugeBadge, "RGBADGE", RainGaugeBadgeStyle, "style");
		lastB = RainGaugeBadge; lastBS = RainGaugeBadgeStyle;
	}
}

// Rain gauge block under the Settings table: live badge + status text.
void appendRainGaugeBlock(AsyncResponseStream* r){
	r->print("<div style='margin:8px 12px;font-size:13px'><b>Irrigation today (rain gauge):</b> <span id='RGBADGE' style='" +
	         RainGaugeBadgeStyle + "'>" + RainGaugeBadge + "</span><br><span id='RGSTAT' style='font-size:12px;color:#475569'>" +
	         RainGaugeStatus + "</span><br><span style='font-size:11px;color:#64748b'>Zones with <i>Respect Rainfall = Yes</i> are skipped when the "
	         "rain gauge says no. Gauge not reachable for 3 h = irrigate as normal.</span></div>");
}
