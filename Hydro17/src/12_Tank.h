// =============================================================================
// 12_Tank.h — Tank Level Sensor Class & Filling Control Declarations
// =============================================================================
// Declares TankLevel (derived from PressureSensor) which manages the tank
// water level sensor, calibration values, and fill/stop thresholds.
// Also declares the runtime flags used by MaintainTankFilling() in 12_Tank.cpp.
// =============================================================================

// ─── Tank Fill Sense Mode Constants ──────────────────────────────────────────
	#define TANK_FILL_SENSE_FLOAT   "Float Switch"   // Use float switches for fill detection
	#define TANK_FILL_SENSE_LEVEL   "Level Sensor"   // Use pressure/level sensor for fill detection

// ─── Tank Fill Operation Mode Constants ──────────────────────────────────────
	#define TANK_FILL_OPER_NORMAL    "Any Time"       // Fill allowed at any time of day
	#define TANK_FILL_OPER_SHEDULE   "Scheduled Time" // Fill only within TankFillStartTime–StopTime

// ─── Runtime Tank State Flags ────────────────────────────────────────────────
	bool TankFillAllowed      = true;   // false when float-low is active (prevents irrigation)
 	bool TankFilterOnOffButton= true;   // true = filter pump button is ready to be pressed
	bool TankFillChange       = false;  // true when fill state has changed (triggers home update)
	bool FloatHigh            = false;  // true = high-level float switch is triggered
	bool LevelHigh            = false;  // true = sensor reading is above stop-fill threshold
	bool FloatLow             = false;  // true = low-level float switch is triggered
	bool AllowFillingAtTime   = true;   // false when outside the scheduled fill window
	bool TankIsFull           = true;   // true = tank is full (float or sensor indicates full)

// ─── Tank Page Row Index Constants ───────────────────────────────────────────
// Each constant is an index into the TankList PageVar array.
// The order here must match the order in MapTankPage().
	#define TANK_RAW_VALUE          0   // Display only — live ADC raw value from level sensor
	#define TANK_SENSOR_100         1   // ADC calibration value for 100% full
	#define TANK_SENSOR_100_FILTER  2   // ADC calibration value for 100% (filtered)
	#define TANK_SENSOR_0           3   // ADC calibration value for 0% (empty)
	#define TANK_FILL_SENSE         4   // Fill detection method: Float Switch or Level Sensor
	#define TANK_LEVEL_START_FILL   5   // Level % at which filling starts
	#define TANK_LEVEL_STOP_FILL    6   // Level % at which filling stops
	#define TANK_FILL_OPER          7   // Fill operation mode: Any Time or Scheduled Time
	#define TANK_SHED_START         8   // Scheduled fill window start time (HH:MM)
	#define TANK_SHED_STOP          9   // Scheduled fill window stop time (HH:MM)
	#define TANK_FILTPUMP             10  // Normal-time filter pump run period (minutes)
	#define TANK_FILTPUMP_RED_FROM    11  // Reduced period window start time (HH:MM)
	#define TANK_FILTPUMP_RED_TO      12  // Reduced period window end time (HH:MM)
	#define TANK_FILTPUMP_RED_DELAY   13  // Filter pump run period during reduced window (minutes)
	#define TANK_KPA_VALUE            14  // Display only — live filter pressure in kPa
	#define TANK_BACKWASH             15  // Backwash pressure warning threshold (kPa)
	#define TANK_SENSOR_MAX           16  // Calibration — ADC value at full-scale pressure
	#define TANK_FILTER_RAW_VALUE     17  // Display only — live ADC raw value from filter sensor
	#define TANK_FILTER_EXCEEDED      18  // Latching alarm — true when BackwashWarning has been exceeded

	#define PAGE_VAR_TANK             19  // Total PageVar entries in TankList

// ─── Page Variable List Pointer ───────────────────────────────────────────────
	PageVar* TankList;  // PageVar list for the Tank page (set in MapTankPage())

// =============================================================================
// TankLevel  (inherits from PressureSensor)
// =============================================================================
// Extends PressureSensor with tank-specific calibration values and fill logic.
//
// Calibration values:
//   RawValueMax     — ADC reading when tank is 100% full (no filter running)
//   RawValueMaxFil  — ADC reading when tank is 100% full (filter pump running)
//   RawValueMin     — ADC reading when tank is 0% empty
//
// Fill thresholds:
//   StartFillLevel  — percentage below which the fill solenoid opens
//   StopFillLevel   — percentage at which the fill solenoid closes
//
// Schedule:
//   TankFillingTime     — TANK_FILL_OPER_NORMAL or TANK_FILL_OPER_SHEDULE
//   TankFillStartTime   — start of allowed fill window (HH:MM)
//   TankFillStopTime    — end of allowed fill window (HH:MM)
//   FilterPumpDelay     — minutes to run the filter pump after filling completes
//
// Data persists to /TankLevelData.json via WriteData()/ReadData().
// =============================================================================
class TankLevel : public PressureSensor
{
public:
    TankLevel(int Pin);  // Constructor — initialises calibration defaults and pin
    ~TankLevel();        // Destructor
	void WriteData();    // Serialise calibration and fill settings to /TankLevelData.json
	void ReadData();     // Load calibration and fill settings from /TankLevelData.json
	void Maintain();              // Sample ADC and update rolling average (overrides PressureSensor — tank-specific only)
	int  getValue();              // Returns tank level as a percentage (0–100) using filtered max
	int  getValueNotFilter();     // Returns tank level as a percentage (0–100) using unfiltered max

	// ── Calibration Values ────────────────────────────────────────────────────
	int RawValueMax      = 7000;  // ADC at 100% full (filter off)
	int RawValueMaxFil   = 6000;  // ADC at 100% full (filter on)
	int RawValueMin      = 500;   // ADC at 0% empty

	// ── Fill Thresholds ───────────────────────────────────────────────────────
	int    StartFillLevel = 70;   // Start filling below this percentage
	int    StopFillLevel  = 100;  // Stop filling at this percentage

	// ── Fill Schedule ─────────────────────────────────────────────────────────
	String TankFillingSense   = TANK_FILL_SENSE_FLOAT;   // Detection method
	String TankFillingTime    = TANK_FILL_OPER_NORMAL;   // Timing constraint
	String TankFillStartTime  = "08:30";  // Scheduled window open
	String TankFillStopTime   = "16:00";  // Scheduled window close
    int    FilterPumpDelay        = 10;       // Minutes to keep filter running after fill (normal hours)
	String ReducedPumpFromTime    = "18:00";  // Start of reduced filter run window
	String ReducedPumpToTime      = "06:00";  // End   of reduced filter run window
	int    ReducedFilterPumpDelay = 10;       // Filter run minutes during reduced window

	// ── Display Variable ──────────────────────────────────────────────────────
	String StrTankRawValue = "";  // Formatted raw ADC string shown on the Tank page

};

	TankLevel *TankWaterLevel;    // Global TankLevel instance (created in CreatePressureSensors())
	PressureSensor *FilterSensor; // Global FilterSensor instance (created in CreatePressureSensors())

//	inline String StrTankRawValueAtt   = "color: white; background-color: #00cccc";  // CSS style for the tank raw-value display cell
//	inline String StrFilterRawValueAtt = "color: white; background-color: #00cccc";  // CSS style for the filter raw-value display cell
	inline String StrTankRawValueAtt   = ATT_GREEN;  // CSS style for the tank raw-value display cell
	inline String StrFilterRawValueAtt = ATT_GREEN;  // CSS style for the filter raw-value display cell