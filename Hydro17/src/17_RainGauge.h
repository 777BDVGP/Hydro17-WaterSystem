// =============================================================================
// 17_RainGauge.h — RAIN_OK_DO_IRRI from the RainGauge_PIO unit
// =============================================================================
// Settings page: "Use Rain Gauge" checkbox + "Rain Gauge IP" (e.g. 192.168.1.124).
// Ported from Prima3_pomphuis 17_RainGauge (2026-11-24).
//
//   Checkbox OFF -> RAIN_OK_DO_IRRI = true (irrigate as normal, gauge ignored).
//   Checkbox ON  -> every RG_POLL_MS (1 hour), and right after the Settings
//                   page is saved, GET <Rain Gauge IP>api/irrigate and copy
//                   its "irrigate" true/false into RAIN_OK_DO_IRRI.
//
// The scheduler (IrrigationShedule, 09_Irrigation.cpp) is unchanged: when a
// zone with "Respect Rainfall = Yes" is due, RAIN_OK_DO_IRRI false skips it.
//
// Gauge not reachable: the last good answer is kept for RG_STALE_MS (3 h);
// after that RAIN_OK_DO_IRRI falls back to true, so the garden is never left
// dry because of a network or gauge fault.
//
// The HTTP GET blocks (up to ~6 s), so — like NtfyTask — it runs in its own
// small FreeRTOS task (RainGaugeTask).  The task only does the GET; loop()
// (MaintainRainGauge) owns RAIN_OK_DO_IRRI, the status text and the logging.
// =============================================================================

	#define RG_POLL_MS        (60UL * 60UL * 1000UL)        // poll every hour
	#define RG_STALE_MS       (3UL * 60UL * 60UL * 1000UL)  // keep last answer this long on failure
	#define RG_HTTP_TIMEOUT   3000                          // ms, connect and read

	String RainGaugeStatus = "Not active";   // shown on the Settings page
	// Green / red badge under the IP row: what RAIN_OK_DO_IRRI is right now.
	String RainGaugeBadge      = "";
	String RainGaugeBadgeStyle = "";
	#define RG_BADGE_GREEN  "display:inline-block;padding:3px 10px;border-radius:5px;font-weight:700;color:#fff;background:#16a34a"
	#define RG_BADGE_RED    "display:inline-block;padding:3px 10px;border-radius:5px;font-weight:700;color:#fff;background:#dc2626"

	// loop() -> task: URL to fetch
	struct RainGaugeReq {
		char url[96];
	};

	// task -> loop(): result of one fetch
	struct RainGaugeRes {
		bool ok;               // HTTP 200 and valid JSON with "irrigate"
		bool irrigate;
		bool sim;              // gauge is in simulation mode
		int  httpCode;         // <0 = connection error (HTTPClient codes)
		char reason[120];
	};

	QueueHandle_t RainGaugeReqQueue   = NULL;
	QueueHandle_t RainGaugeResQueue   = NULL;
	TaskHandle_t  RainGaugeTaskHandle = NULL;
	volatile bool RainGaugePollNow    = true;   // set by Settings save -> poll at once
