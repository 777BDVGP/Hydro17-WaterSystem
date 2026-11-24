// =============================================================================
// 09_Irrigation.h — Irrigation Scheduling Classes & Declarations
// =============================================================================
// Declares IrrigateSelection (per-zone configuration), Irrigation (the main
// zone manager with JSON persistence), and SheduleIrri (the runtime schedule
// queue that tracks which zones are active, pending, or completed).
// IRRIGATE_SELECTIONS defines both the number of zones and the schedule depth.
// =============================================================================

// ─── Zone Count & Index Constants ────────────────────────────────────────────
	#define IRRIGATE_SELECTIONS     9   // Total number of irrigation zones (also schedule depth)
	#define IRRIGATE_SEL_1          0
	#define IRRIGATE_SEL_2          1
	#define IRRIGATE_SEL_3          2
	#define IRRIGATE_SEL_4          3
	#define IRRIGATE_SEL_5          4
	#define IRRIGATE_SEL_6          5
	#define IRRIGATE_SEL_7          6
	#define IRRIGATE_SEL_8          7
	#define IRRIGATE_SEL_9          8
	#define IRRIGATE_TANK_FILL      9   // Synthetic entry used for tank-fill scheduling

// ─── Status & Mode String Constants ──────────────────────────────────────────
	#define IRRI_STATUS_ACTIVE      "Active"
	#define IRRI_STATUS_INACTIVE    "Not Active"

	#define IRRI_MODE_MANUAL        "Manual"       // Trigger manually via web page
	#define IRRI_MODE_SHEDULE       "Schedule"     // Trigger automatically on schedule

	#define IRRI_SOURCE_TANK        "Tank"         // Use tank supply solenoid
	#define IRRI_SOURCE_BOLEHOLE    "Borehole"     // Use borehole pump

	#define IRRI_RESP_RAIN_YES      "Yes"          // Skip zone if rain detected
	#define IRRI_RESP_RAIN_NO       "No"           // Irrigate regardless of rain

	#define IRRI_STATUS_START       "START"        // Manual start button label
	#define IRRI_STATUS_STOP        "BUSY - STOP"  // Manual stop button label (while running)
	#define IRRI_STATUS_SHED        "SCHEDULED"    // Scheduled zone indicator label

// ─── Suspend Day String Constants ────────────────────────────────────────────
// Defined here for the IrrigateSelection per-day suspend fields.
// (Note: 09_Suspend.h redefines SUSPEND_YES/NO as bool for the SuspendDays class.)
	#undef  SUSPEND_YES
	#undef  SUSPEND_NO
	#define SUSPEND_YES  "Yes"
	#define SUSPEND_NO   "No"

// ─── Day-of-Week Constants ────────────────────────────────────────────────────
	#define DAY_SUNDAY    1
	#define DAY_MONDAY    2
	#define DAY_TUEDAY    3
	#define DAY_WEDDAY    4
	#define DAY_THUDAY    5
	#define DAY_FRIDAY    6
	#define DAY_SATDAY    7

// ─── Web Page State ───────────────────────────────────────────────────────────
	int CurrentIrriSetup = 0;  // Index of the zone currently shown in the setup form (0-based)

// =============================================================================
// IrrigateSelection
// =============================================================================
// Stores the user-configurable settings for one irrigation zone.  An array of
// IRRIGATE_SELECTIONS instances lives inside the Irrigation object.
//
// Key fields:
//   Status           — whether this zone is active (user enabled/disabled)
//   ConfigLine       — which valve configuration to activate for this zone
//   OperMode         — Manual (web-button) or Schedule (automatic)
//   NextDateToIrrigate / NextTimeToIrrigate — next scheduled run date and time
//   PeriodToIrrigate — how long to run (HH:MM format)
//   IrrigationFreq   — recurrence in days
//   WaterSource      — Tank or Borehole
//   SuspendXxx       — per-day suspension flags ("Yes" / "No")
// =============================================================================
class IrrigateSelection
{
public:
	String GetPeriodToIrrigateSeconds();  // Returns PeriodToIrrigate converted to seconds

	// ── Zone Configuration Fields ──────────────────────────────────────────────
	String SelectDescription   = "Not Used";
	String Status              = IRRI_STATUS_INACTIVE;
	String ConfigLine          = CONFIG_NONE_STR;
	String OperMode            = IRRI_MODE_MANUAL;
	String NextDateToIrrigate  = "2024-01-01";
	String NextTimeToIrrigate  = "06:00";
	String PeriodToIrrigate    = "00:10";
	int    IrrigationFreq      = 1;             // Recurrence in days
	String WaterSource         = IRRI_SOURCE_BOLEHOLE;
	String RespectRainfail     = IRRI_RESP_RAIN_NO;

	// ── Per-Day Suspension Fields ─────────────────────────────────────────────
	String SuspendMon = SUSPEND_NO;
	String SuspendTue = SUSPEND_NO;
	String SuspendWed = SUSPEND_NO;
	String SuspendThu = SUSPEND_NO;
	String SuspendFri = SUSPEND_NO;
	String SuspendSat = SUSPEND_NO;
	String SuspendSun = SUSPEND_NO;
 	String SuspendALL = SUSPEND_NO;   // Overrides all per-day flags

	// ── Web Page Element IDs ──────────────────────────────────────────────────
	int SelectDescriptionID;
	int StatusID;
	int ConfigLineID;
	int OperModeID;
	int NextDateToIrrigateID;
	int NextTimeToIrrigateID;
	int PeriodToIrrigateID;
	int IrrigationFreqID;
	int WaterSourceID;
	int RespectRainfailID;
	int SuspendMonID;
	int SuspendTueID;
	int SuspendWedID;
	int SuspendThuID;
	int SuspendFriID;
	int SuspendSatID;
	int SuspendSunID;
	int SuspendALLID;
};

// =============================================================================
// Irrigation
// =============================================================================
// Manages the array of IRRIGATE_SELECTIONS irrigation zones.  Handles JSON
// serialisation (ReadData/WriteData), web page maintenance, and exposes
// BusyPumpingConfig to indicate which valve configuration is currently active
// (-1 = none active).
// =============================================================================
class Irrigation
{
public:
	Irrigation();               // Constructor — allocates and initialises zone array
	~Irrigation();              // Destructor  — frees allocated resources
	void   Maintain();          // Update runtime state from zone array
	String GetDescription(int Line); // Human-readable zone description for display
	String GetOption(int Line);      // Zone option string for drop-down selects
	void   WriteData();         // Serialise all zone data to Irrigation.json
	void   ReadData();          // Load zone data from Irrigation.json

	int BusyPumpingConfig = -1; // Index of the currently active valve config (-1 = idle)

	IrrigateSelection Selection[IRRIGATE_SELECTIONS]; // All zone configurations
};

	Irrigation *IrriObject;   // Global Irrigation instance (created in CreateIrrigation())

// =============================================================================
// Scheduling System
// =============================================================================
// SheduleIrri holds the runtime queue of zone irrigation events.  Each entry
// in IrrigateShedule[] tracks whether its zone is Scheduled (S), Active (A),
// or Deactivated (D).
// =============================================================================

	#define Lines  IRRIGATE_SELECTIONS  // Schedule depth equals number of zones

	#define SHED_TYPE_AUTO    "A"  // Automatically triggered by the schedule
	#define SHED_TYPE_MANUAL  "M"  // Manually triggered via the web page

	#define SHED_STATUS_SCHEDULED  "S"  // Waiting for its start time
	#define SHED_STATUS_ACTIVE     "A"  // Currently irrigating
	#define SHED_STATUS_DEACTIVE   "D"  // Completed or cancelled

// ─────────────────────────────────────────────────────────────────────────────
// SheduleLines
// Tracks the runtime state of one scheduled irrigation zone.
// ─────────────────────────────────────────────────────────────────────────────
class SheduleLines
{
public:
	String        ShedStat;       // Current state: SHED_STATUS_SCHEDULED / ACTIVE / DEACTIVE
	String        StopDateTime;   // Calculated stop date-time (YYYY-MM-DD HH:MM) — kept for display
	unsigned long StopMillis = 0; // millis() deadline for stopping — used for accurate duration control
	bool          ManualStop;     // true = user has requested early stop via web button
};

// ─────────────────────────────────────────────────────────────────────────────
// SheduleIrri
// Holds the full schedule queue for all irrigation zones.
// Init() resets all entries to SHED_STATUS_DEACTIVE at startup.
// ─────────────────────────────────────────────────────────────────────────────
class SheduleIrri
{
public:
	SheduleIrri();   // Constructor
	~SheduleIrri();  // Destructor
	void Init();     // Reset all schedule entries to deactivated state

 	SheduleLines IrrigateShedule[Lines]; // Runtime queue — one entry per zone
};

	SheduleIrri *SheduleObject;  // Global schedule instance (created in CreateSheduleObj())
