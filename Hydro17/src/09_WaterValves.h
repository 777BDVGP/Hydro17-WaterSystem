// =============================================================================
// 09_WaterValves.h — Electric Valve Classes & Configuration Declarations
// =============================================================================
// Declares the ElectricValves class that manages all solenoid valves and their
// switching configurations.  Each valve has an active state, a physical pin,
// and configurable on/off switch delays.  Up to VALVE_OUTPUT_CONFIGS named
// configurations group valves for coordinated irrigation runs.
// =============================================================================

// ─── Valve Relevance Constants ────────────────────────────────────────────────
// Determines which subsystem a valve configuration belongs to.
	#define REL_IRRILINE  1  // Configuration is used by the irrigation scheduler
	#define REL_TANK      2  // Configuration is used by the tank filling system

// ─── Valve State Constants ────────────────────────────────────────────────────
	#define VALVE_ACTIVE    true   // Valve is in service (can be operated)
	#define VALVE_INACTIVE  false  // Valve is decommissioned / not used

	#define VALVE_OPEN      true   // Solenoid energised — valve open
	#define VALVE_CLOSE     false  // Solenoid de-energised — valve closed

// ─── State Machine Request Constants ─────────────────────────────────────────
// Used by MaintainValveStatus() to track pending open/close transitions.
	#define STATE_OPEN_REQ   2  // Open request pending in the relay state machine
	#define STATE_CLOSE_REQ  4  // Close request pending in the relay state machine

// ─── Valve Array Size Constants ───────────────────────────────────────────────
	#define SOL_VALVE_DISPLAY   6  // Number of user-visible solenoid valves (1–6)
	#define SOL_OUTPUT_COUNT    9  // Total solenoid outputs including tank/borehole

	// Individual valve output indices
	#define SOL_VALVE_1   0
	#define SOL_VALVE_2   1
	#define SOL_VALVE_3   2
	#define SOL_VALVE_4   3
	#define SOL_VALVE_5   4
	#define SOL_VALVE_6   5
 	#define SOL_TANK_FILL 6  // Tank fill solenoid (not shown on valve names page)
 	#define SOL_TANK_SUP  7  // Tank supply solenoid (not shown on valve names page)
 	#define SOL_BOREHOLE  8  // Borehole pump relay (not shown on valve names page)

// ─── Configuration Array Size Constants ──────────────────────────────────────
	#define VALVE_DISPLAY_CONFIGS  9   // Number of user-visible configurations (1–9)
	#define VALVE_OUTPUT_CONFIGS   10  // Total configurations including tank-fill config

	// Individual configuration indices
	#define CONFIG_1   0
	#define CONFIG_2   1
	#define CONFIG_3   2
	#define CONFIG_4   3
	#define CONFIG_5   4
	#define CONFIG_6   5
	#define CONFIG_7   6
	#define CONFIG_8   7
	#define CONFIG_9   8
	#define CONFIG_10  9  // Reserved for tank fill-up (not displayed on web page)

// ─── Configuration Name String Constants ─────────────────────────────────────
	#define CONFIG_NONE_STR  "None"
	#define CONFIG_1_STR     "Config 1"
	#define CONFIG_2_STR     "Config 2"
	#define CONFIG_3_STR     "Config 3"
	#define CONFIG_4_STR     "Config 4"
	#define CONFIG_5_STR     "Config 5"
	#define CONFIG_6_STR     "Config 6"
	#define CONFIG_7_STR     "Config 7"
	#define CONFIG_8_STR     "Config 8"
	#define CONFIG_9_STR     "Config 9"
	#define CONFIG_10_STR    "Tank  10"  // Tank fill-up config — not displayed

// ─── Valve Switch Timing ──────────────────────────────────────────────────────
// Delays applied by MaintainValveStatus() when opening or closing valves.
	#define VALVE_SWITCH_DELAY_1   50    // 50 ms — default inter-valve switch delay
	#define VALVE_SWITCH_DELAY_2   4000  // 4 s   — extended delay for pump start sequencing

// ─── Runtime Valve State Variables ───────────────────────────────────────────
	bool NextSheduleInQueIsBorhole = false; // true when the next queued zone uses the borehole
	unsigned long StartValveTime;           // millis() timestamp when the current valve opened
	String StartPerc = "";                  // Tank level percentage at valve-open time (logged)
	bool ValveLastState = VALVE_CLOSE;      // Last commanded valve state (used by state machine)

// =============================================================================
// ValveConfigActiveState
// =============================================================================
// Tracks whether a specific valve output is active (on) for a given configuration.
//   State   — true = this valve should be open for this configuration
//   StateID — web page element ID for this valve's checkbox in the config editor
// =============================================================================
class ValveConfigActiveState
{
public:
	bool State   = VALVE_INACTIVE; // true = valve is part of this configuration
	int  StateID = 0;              // Web page checkbox element ID
};

// =============================================================================
// ValveAtrributes
// =============================================================================
// All physical and timing attributes for one solenoid valve output.
//   Description      — user-assigned name (from the Valve Names page)
//   RequiredState    — the state the valve should be in (commanded)
//   ActualState      — the state the valve hardware is currently in
//   UpdateTime       — millis() timestamp of the last state transition
//   OutputPin        — GPIO pin number driving the relay/solenoid
//   SwitchDelayOn/Off — inter-valve sequencing delay in ms
// =============================================================================
class ValveAtrributes
{
public:
	String        Description   = "";              // User-assigned valve name
	bool          RequiredState = VALVE_CLOSE;     // Commanded state
	bool          ActualState   = VALVE_CLOSE;     // Current hardware state
	unsigned long UpdateTime    = 0;               // Last state-change timestamp (ms)
 	int           OutputPin     = 0;               // GPIO output pin

	// Switch sequencing delays (configurable per valve via web page)
    int SwitchDelayOn    = VALVE_SWITCH_DELAY_1;  // Delay before opening
    int SwitchDelayOff   = VALVE_SWITCH_DELAY_1;  // Delay before closing
    int SwitchDelayOnID  = 0;                     // Web page element ID for on-delay
    int SwitchDelayOffID = 0;                     // Web page element ID for off-delay
};

// =============================================================================
// ValveConfigurations
// =============================================================================
// One named group of valve states used as an irrigation or tank-fill recipe.
//   Relevance   — REL_IRRILINE or REL_TANK (determines which subsystem uses it)
//   ConfigName  — user-assigned configuration name
//   StartDateTime — when this configuration last became active (logged)
//   ValveStatus[] — per-output active/inactive flag for this configuration
// =============================================================================
class ValveConfigurations
{
public:
	int     Relevance   = REL_IRRILINE; // Which subsystem owns this configuration
	String  ConfigName  = "";           // User-assigned name (e.g. "Front Garden")
	String  StartDateTime = "";         // Activation timestamp for logging

	ValveConfigActiveState ValveStatus[SOL_OUTPUT_COUNT]; // Per-output active flags
};

// =============================================================================
// ElectricValves
// =============================================================================
// Top-level valve manager.  Owns the WaterValve[] array (physical attributes)
// and the ValveConfig[] array (named switching recipes).  Persists both to
// /WaterValves.json via WriteData()/ReadData().
// =============================================================================
class ElectricValves
{
public:
	ElectricValves();   // Constructor — sets up default names and pin assignments
	~ElectricValves();  // Destructor
	void Init();              // Initialise valve attributes from default values
	void SwitchValveConfig(); // Apply a configuration — set RequiredState for each valve
	void WriteData();         // Serialise to /WaterValves.json
	void ReadData();          // Load from /WaterValves.json

	ValveAtrributes    WaterValve[SOL_OUTPUT_COUNT];          // Physical valve attributes
	ValveConfigurations ValveConfig[VALVE_OUTPUT_CONFIGS];    // Named switching configurations
};

// ─── Global Instances & Page Variable Pointers ───────────────────────────────
	ElectricValves *ElecValves;   // Global valve manager (created in CreateElectricValves())
	PageVar* ElecValvesList;      // PageVar list for the Valve Names page
    PageVar* ConfigList;          // PageVar list for the Valve Configurations page
