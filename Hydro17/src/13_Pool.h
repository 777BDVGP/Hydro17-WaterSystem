// =============================================================================
// 13_Pool.h — Pool Filter Pump Schedule & Light Control Declarations
// =============================================================================
// Declares the PoolShedule class that manages per-day filter pump schedules,
// and the runtime state flags for the pool filter pump and pool light.
// MaintainPoolFilterPump() in 13_Pool.cpp evaluates these flags every 10 ms.
// =============================================================================

// ─── Pool Page Row Index Constants ───────────────────────────────────────────
// Each constant is an index into the PoolFilter schedule display.
	#define POOL_START    0  // Schedule start time field index
	#define POOL_STOP     1  // Schedule stop time field index
	#define POOL_SELECT   2  // Day enable/disable checkbox field index

	#define PAGE_VAR_POOL 3  // Total PageVar entries per pool schedule day
	#define WEEK_DAYS     7  // Number of days in the weekly schedule

// ─── Pool Filter Override State ───────────────────────────────────────────────
// Replaces the old PoolManual bool with a three-state override that encodes
// why the pump is being held on or off outside the weekly schedule.
//   NONE        — weekly schedule is in control
//   FORCED_OFF  — pump held off until midnight (user switched it off manually)
//   FORCED_ON   — pump held on until 19:00, then transitions to FORCED_OFF
// All states reset to NONE at midnight via CompileDateTime().
	enum PoolOverrideState { POOL_OVR_NONE, POOL_OVR_FORCED_OFF, POOL_OVR_FORCED_ON };
	PoolOverrideState PoolOverride = POOL_OVR_NONE;

// ─── Runtime Pool State Flags ─────────────────────────────────────────────────
	bool POOL_FILTER_ON  = false;  // true = pool filter pump is currently running
	bool POOL_LIGHT_ON   = false;  // true = pool light relay is currently active

	bool POOL_FILTER_BUT = true;   // true = filter pump toggle button is ready to be pressed
	bool POOL_LIGHT_BUT  = true;   // true = pool light toggle button is ready to be pressed

	bool LightManual = false;      // true = pool light is running in manual override mode
	                               //        (cleared at midnight by CompileDateTime)

// ─── Day Label Array ──────────────────────────────────────────────────────────
	String DayDesc[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    String IDStyle   = "";  // CSS style string for pool schedule page ID elements

// =============================================================================
// PoolSheduleAttr
// =============================================================================
// Configuration for one day's pool filter schedule.
//   Start  — filter-on time (HH:MM format)
//   Stop   — filter-off time (HH:MM format)
//   Select — true = scheduling is enabled for this day
// =============================================================================
class PoolSheduleAttr
{
public:
    String Start  = "08:00";  // Scheduled pump-on time
    String Stop   = "16:00";  // Scheduled pump-off time
	bool   Select = false;    // true = this day's schedule is active
};

// =============================================================================
// PoolShedule
// =============================================================================
// Manages the seven-day pool filter pump schedule.  Data persists to
// /PoolFilter.json via WriteData()/ReadData().
// MaintainPoolFilterPump() reads Scheduleday[WeekDay - 1] to decide whether
// to run the filter pump.
// =============================================================================
class PoolShedule
{
public:
	PoolShedule();   // Constructor — initialises default schedule for all seven days
	~PoolShedule();  // Destructor
	void WriteData(); // Serialise schedule to /PoolFilter.json
	void ReadData();  // Load schedule from /PoolFilter.json

	PoolSheduleAttr Scheduleday[SUSPEND_DAYS]; // Per-day schedule: Scheduleday[0]=Sun … [6]=Sat
};

	PoolShedule *PoolFilter;  // Global pool filter schedule instance (created in CreatePoolShedule())

// =============================================================================
// PoolLightShedule
// =============================================================================
// Manages the seven-day pool light schedule.  Data persists to
// /PoolLight.json via WriteData()/ReadData().
// MaintainPoolFilterPump() reads Scheduleday[WeekDay - 1] to decide whether
// to switch the pool light on or off.
// =============================================================================
class PoolLightShedule
{
public:
	PoolLightShedule();   // Constructor
	~PoolLightShedule();  // Destructor
	void WriteData();     // Serialise schedule to /PoolLight.json
	void ReadData();      // Load schedule from /PoolLight.json

	PoolSheduleAttr Scheduleday[WEEK_DAYS]; // Per-day schedule: Scheduleday[0]=Sun … [6]=Sat
};

	PoolLightShedule *PoolLight;  // Global pool light schedule instance (created in CreatePoolLightShedule())
