// =============================================================================
// 12_AnalogRead.h — Pressure Sensor Base Class Declaration
// =============================================================================
// Declares PressureSensor, the base class for analog sensor inputs.
// It accumulates a rolling average of TANK_AVE_OF readings to smooth out
// ADC noise before the value is used for tank level decisions.
// TankLevel (12_Tank.h) inherits from PressureSensor.
// =============================================================================

	#define TANK_AVE_OF  15  // Number of ADC samples in the rolling average

	// Consecutive ticks above BackwashWarning required to latch FilterPressureExceeded.
	// 17 × 300 ms ≈ 5 seconds of sustained over-threshold pressure.
	#define FILTER_EXCEED_TICKS  17

// =============================================================================
// PressureSensor
// =============================================================================
// Base class for analog pressure/level sensors.  Stores the last TANK_AVE_OF
// raw ADC readings and exposes a smoothed average through Maintain().
//
// Derived class TankLevel extends this with calibrated range values and the
// full tank fill/stop logic.
// =============================================================================
class PressureSensor {
public:
	PressureSensor(int Pin);  // Constructor — sets IOPin and initialises the buffer
	~PressureSensor();        // Destructor
	virtual void Maintain(); // Read ADC, store in ring buffer, update RawValue average
	virtual int  getValue(); // Return current pressure in kPa (0–103.4 kPa, 0–15 PSI range)
	void WriteData();        // Serialise calibration settings to /FilterSensorData.json
	void ReadData();         // Deserialise calibration settings from /FilterSensorData.json

	// ── Variables ─────────────────────────────────────────────────────────────
	int      IOPin        = 0;      // GPIO / ADC pin number for this sensor
	int      TimerIdx     = 0;      // Timer index used for the averaging tick
	long int RawValue     = 0;      // Current averaged ADC reading (0–8191, ESP32-S2 13-bit ADC)
	int      RawValueMax  = 8191;   // ADC value at full-scale (400 kPa) — ESP32-S2 13-bit ADC

	long int Idx     = -1;               // Ring-buffer write index (-1 = not yet filled)
	long int AveList[TANK_AVE_OF];       // Rolling ADC sample buffer

	String StrFilterKPa    = "0 kPa";  // Formatted kPa string for display (updated by Maintain())
	String StrFilterRawValue = "0"; // Raw ADC average for display (updated by Maintain())
	int    BackwashWarning        = 80;    // Alert threshold in kPa — home page turns red above this value
	bool   FilterPressureExceeded = false; // Latching flag — set true when BackwashWarning is exceeded for FILTER_EXCEED_TICKS; must be manually cleared
	int    pressureExceedCount    = 0;     // Consecutive ticks above BackwashWarning; alarm latches at FILTER_EXCEED_TICKS
};
