// =============================================================================
// 12_Settings.h — System Parameters Class & Settings Page Declarations
// =============================================================================
// Declares AllParams, the central configuration store for system-wide settings
// that persist across reboots in /AllParams.json (via SPIFFS).
// Also declares the Settings page variable list and a shared static page buffer
// used by several page-build functions to avoid heap fragmentation.
// =============================================================================

// =============================================================================
// AllParams
// =============================================================================
// Stores all user-configurable system parameters.  ReadData() loads them from
// SPIFFS on startup; WriteData() saves them when the Settings page is submitted.
//
// Key fields:
//   SysName                  — controller name shown in the page title
//   LogXxxEvents             — maximum log file entries before trimming
//   WatchdogTime             — seconds before watchdog forces a reboot
//   ResetTimeOnContDisabled  — minutes before auto-restore to normal operation
//   NextXxxDate              — scheduled maintenance due dates (YYYY-MM-DD)
//   NormalOperationSaved     — persists the last NormalOperation state
//   KeepAliveEnabled         — gateway keepalive ping on/off
//   MidnightRebootEnabled    — scheduled nightly reboot on/off
//   MidnightRebootTime       — time of that reboot, "HH:MM" (default 00:05)
// =============================================================================
class AllParams
{
public:
	AllParams();   // Constructor — initialises default values
	~AllParams();  // Destructor
	void Maintain();   // Reserved for periodic parameter checks (currently unused)
	void WriteData();  // Serialise all parameters to /AllParams.json
	void ReadData();   // Load all parameters from /AllParams.json

	// ── System Identification ──────────────────────────────────────────────────
	String SysName = "V5 Water Controller";

	// ── Log File Retention ────────────────────────────────────────────────────
	int LogGeneralEvents    = 14;   // Maximum entries in GeneralLog.txt
	int LogIrrigationEvents = 14;   // Maximum entries in IrrigationLog.txt
	int LogTankFillingEvents= 14;   // Maximum entries in TankFillingLog.txt

	// ── Watchdog & Connectivity Timeouts ─────────────────────────────────────
	int WatchdogTime            =  5;   // Seconds without heartbeat before reboot
	int ResetTimeOnContDisabled =  60;  // Minutes in disabled mode before auto-restore

	// ── State Persistence ─────────────────────────────────────────────────────
	bool NormalOperationSaved = true;  // Last saved NormalOperation state

	// ── Gateway Keepalive (MaintainGatewayWatch, 01_Main.cpp) ───────────────
	// Pings the gateway (router) every 30 s so the Omada network never ages
	// this unit out; reboots after 5 min with no reply.
	bool   KeepAliveEnabled = true;    // Keepalive ping + reboot guard on/off

	// ── Scheduled Nightly Reboot (MaintainMidnightReboot, 02_Time.cpp) ───────
	bool   MidnightRebootEnabled = true;   // Reboot every night at MidnightRebootTime
	String MidnightRebootTime    = "00:05";  // "HH:MM" - set on SETTINGS (time field next to the tick box)

	// ── Rain gauge (17_RainGauge.cpp) — sets RAIN_OK_DO_IRRI hourly ─────────
	bool   RainGaugeEnable  = false;                  // false -> RAIN_OK_DO_IRRI always true
	String RainGaugeURL     = "http://192.168.1.124/"; // Rain Gauge unit (RainGauge project)

	// ── ABOUT page ──────────────────────────────────────────────────────────
	bool   ShowOtaInfo      = false;   // ABOUT: show the PC-upload (ArduinoOTA) rows — upload works either way

private:
	// No private members — all settings are public for direct web-page binding
};
	AllParams *Params;   // Global instance (created in CreateAllParams())

// ─── Settings Page Row Index Constants ───────────────────────────────────────
// Each constant is an index into the SettingsList PageVar array.
// The order here must match the order in MapSettingsPage().
	#define SET_SYSNAME               0
	#define SET_LOG_GENERAL           1
	#define SET_LOG_IRRIGATION        2
	#define SET_LOG_TANKFILLING       3
	#define SET_WATCHDOG_TIME         4
	#define SET_CONT_DISABLE_RESET    5
	#define SET_KEEPALIVE_ENABLE      6
	#define SET_MIDNIGHT_REBOOT       7
	#define SET_RAINGAUGE_EN          8   // Use Rain Gauge checkbox
	#define SET_RAINGAUGE_URL         9   // Rain Gauge IP
	#define SET_SHOW_OTA             10   // ABOUT: show PC-upload (OTA) rows
	#define SET_REBOOT_TIME          11   // nightly reboot time (shown in the SET_MIDNIGHT_REBOOT row)

	#define PAGE_VAR_SETTINGS     12   // Total PageVar entries in SettingsList

// ─── Static Page Build Buffer ─────────────────────────────────────────────────
// 8 KB fixed buffer used by several page builders as an alternative to
// String concatenation, eliminating repeated heap allocations on large pages.
// resetPageBuf() clears it; appendToPageBuf() writes to it; send via PageBuf.
// Lives in PSRAM (2026-11-24, new core): 8 KB of internal RAM back to the heap.
// Allocated on first use; falls back to internal RAM if PSRAM is missing.
	#define PAGE_BUF_SIZE 8000
	char*  PageBuf    = nullptr;
	size_t PageBufPos = 0;
	inline void   ensurePageBuf()                  { if (!PageBuf){ PageBuf = (char*)ps_malloc(PAGE_BUF_SIZE); if (!PageBuf) PageBuf = (char*)malloc(PAGE_BUF_SIZE); PageBuf[0] = 0; } }
	inline void   resetPageBuf()                   { ensurePageBuf(); PageBufPos = 0; PageBuf[0] = 0; }
	inline void   appendToPageBuf(const char *s)   { ensurePageBuf(); while (*s && PageBufPos + 1 < PAGE_BUF_SIZE) PageBuf[PageBufPos++] = *s++; }
	inline void   appendToPageBuf(const String &s) { ensurePageBuf(); for (size_t i = 0; i < s.length() && PageBufPos + 1 < PAGE_BUF_SIZE; ++i) PageBuf[PageBufPos++] = s[i]; }
	inline size_t getPageBufLen()                  { ensurePageBuf(); PageBuf[PageBufPos] = 0; return PageBufPos; }

// ─── Settings Page Variables ──────────────────────────────────────────────────
	PageVar* SettingsList;  // PageVar list for the Settings page (set in MapSettingsPage())
	String   SetStyle = ""; // CSS style string for the Settings page value column
