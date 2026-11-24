// =============================================================================
// 09_Suspend.h — Irrigation Suspension Day Classes & Declarations
// =============================================================================
// Declares the SuspendDays class that stores per-day suspension flags for the
// irrigation system.  SUSPEND_IRRI and RAIN_OK_DO_IRRI are the runtime flags
// evaluated by IrrigationShedule() before each irrigation run.
// =============================================================================

// ─── Suspend Boolean Constants ────────────────────────────────────────────────
// Used as initial values for SuspendAttributes fields.
// (Note: 09_Irrigation.h redefines these as "Yes"/"No" strings for IrrigateSelection.)
	#define SUSPEND_YES   true
	#define SUSPEND_NO    false

// ─── Suspend Day Index Constants ─────────────────────────────────────────────
// Index values into SuspendDays::SuspendingDay[].
	#define SUSPEND_DAYS  7   // Total suspension day entries (one per day of the week)
	#define SUSPEND_1     0   // Day slot 1
	#define SUSPEND_2     1   // Day slot 2
	#define SUSPEND_3     2   // Day slot 3
	#define SUSPEND_4     3   // Day slot 4
	#define SUSPEND_5     4   // Day slot 5
	#define SUSPEND_6     5   // Day slot 6
	#define SUSPEND_7     6   // Day slot 7

// ─── Runtime Suspension Flags ────────────────────────────────────────────────
// Evaluated by IrrigationShedule() before each run.
	bool SUSPEND_IRRI    = false;  // true = all irrigation currently suspended
	bool RAIN_OK_DO_IRRI = true;   // false = rain detected; skip rain-sensitive zones

// =============================================================================
// SuspendAttributes
// =============================================================================
// Configuration for one suspension day slot.
//   Day     — day-of-week label (e.g. "Monday")
//   Weekday — numeric day (1 = Sunday … 7 = Saturday), matched against WeekDay
//   Suspend — true = irrigate is suspended on this day
//   Once    — true = suspension applies only once, then auto-clears
// =============================================================================
class SuspendAttributes
{
public:
 	String Day     = "";          // Day name displayed on the Suspend page
	int    Weekday = 0;           // Numeric weekday (1 = Sun … 7 = Sat)
	bool   Suspend = SUSPEND_NO;  // true = irrigation suspended this day
	bool   Once    = SUSPEND_NO;  // true = one-shot suspension (clears after triggering)
};

// =============================================================================
// SuspendDays
// =============================================================================
// Manages the seven-entry suspension day array.  Persists to SuspendDays.json
// via WriteData()/ReadData() and is evaluated each midnight in MaintainSuspention().
// =============================================================================
class SuspendDays
{
public:
	SuspendDays();   // Constructor — initialises default day labels and weekday values
	~SuspendDays();  // Destructor
	void Init();     // Set up day labels and weekday numbers after construction
	void WriteData(); // Serialise suspension settings to /SuspendDays.json
	void ReadData();  // Load suspension settings from /SuspendDays.json

	SuspendAttributes SuspendingDay[SUSPEND_DAYS]; // One entry per day of the week
};

// ─── Global Instances & Page Variable Pointer ────────────────────────────────
	SuspendDays *DaysSuspend;  // Global SuspendDays instance (created in CreateSuspendDays())
	PageVar* SuspendList;      // PageVar array for the Suspend web page (set in MapSuspendPage())
