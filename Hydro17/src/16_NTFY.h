// =============================================================================
// 16_NTFY.h — NTFY.sh Push Notification Parameters & Page Declarations
// =============================================================================
// Declares NtfyParams, the configuration store for push-notification settings
// and maintenance due dates, persisted in /NTFY.json (SPIFFS).
// Also declares the NtfyMsg FreeRTOS queue struct, the NTFY page variable list,
// and all supporting constants.
// =============================================================================

// ─── NtfyMsg — FreeRTOS queue message struct ──────────────────────────────────
// Copied into NtfyQueue at send time — no heap allocation required at send time.
struct NtfyMsg { char message[256]; char priority[16]; char code[64]; };

// =============================================================================
// NtfyParams
// =============================================================================
// Stores NTFY.sh notification settings and scheduled maintenance due dates.
// ReadData() loads from /NTFY.json on startup; WriteData() saves on update.
//
// Key fields:
//   NtfyCode           — NTFY topic/channel code (ntfy.sh/<code>)
//   NtfyPriority       — min / low / default / high / urgent
//   NtfyMessageTime    — HH:MM — daily overdue-alert send time
//   Next*Date          — scheduled maintenance due dates (YYYY-MM-DD)
// =============================================================================
class NtfyParams
{
public:
	NtfyParams();
	~NtfyParams();
	void WriteData();   // Serialise all parameters to /NTFY.json
	void ReadData();    // Load all parameters from /NTFY.json

	// ── NTFY.sh Notification Settings ─────────────────────────────────────────
	String NtfyCode        = " ";          // NTFY topic/channel code
	String NtfyPriority    = "default";    // min / low / default / high / urgent
	String NtfyMessageTime = "08:00";      // HH:MM — daily overdue-alert send time

	// ── Maintenance Due Dates ─────────────────────────────────────────────────
	String NextPoolBackwashDate   = "2024-07-01";
	String NextTankBackwashDate   = "2024-07-01";
	String NextBHCleanDate        = "2024-07-01";
	String NextBlueFilterDate     = "2024-07-01";
	String NextUVTubeReplaceDate  = "2024-07-01";
	String NextChlorineMaintDate  = "2024-07-01";

	// ── Days-to-Add defaults for the Apply table ──────────────────────────────
	// Saved in NTFY.json so the last-used value is remembered across reboots.
	int DaysPoolBackwash  =  14;
	int DaysTankBackwash  =  21;
	int DaysFlushHydro    =   7;
	int DaysBlueFilter    =  84;
	int DaysUVTube        = 365;
	int DaysChlorine      =   7;
};
	NtfyParams *NtfyData;   // Global instance (created in CreateNtfyParams())

// ─── NTFY Page Row Index Constants ───────────────────────────────────────────
// Each constant is an index into the NtfyList PageVar array.
// The order here must match the order in MapNtfyPage().
	#define NTFY_DATE_BACKWASH_POOL    0
	#define NTFY_DATE_BACKWASH_TANK    1
	#define NTFY_DATE_CLEAN_BOREHOLE   2
	#define NTFY_DATE_REPL_BLUE_FILTER 3
	#define NTFY_DATE_REPLACE_UV_TUBE  4
	#define NTFY_DATE_CHLORINE_MAINT   5
	#define NTFY_CODE                  6
	#define NTFY_PRIORITY              7
	#define NTFY_TIME                  8

	#define PAGE_VAR_NTFY              9   // Total PageVar entries in NtfyList
	#define PAGE_VAR_NTFY_DATES_START  0   // First date-field index (NTFY_DATE_BACKWASH_POOL)

// ─── NTFY Page Variables ─────────────────────────────────────────────────────
	PageVar* NtfyList;
	String   NtfyStyle = "";
	bool     NtfyPageObject = false;

// ─── Last-sent date tracker ───────────────────────────────────────────────────
// RAM only — resets on every boot.  Prevents duplicate alerts within one day.
	String NtfyLastSentDate = "";
