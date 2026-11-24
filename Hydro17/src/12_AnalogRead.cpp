// =============================================================================
// 12_AnalogRead.cpp — PressureSensor Class Implementation
// =============================================================================
// Implements the PressureSensor base class.  Maintain() is called every
// TIM_PRESSURE_SENSOR (300 ms) to read the ADC and update a rolling average of
// TANK_AVE_OF (15) samples, smoothing out noise before the value is used
// for tank level percentage calculations and filter pressure monitoring.
// =============================================================================

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
// PressureSensor  (Constructor / Destructor)
// Stores the ADC pin and leaves the ring buffer uninitialised; Maintain()
// fills the entire buffer on the first call (Idx == -1).
// ─────────────────────────────────────────────────────────────────────────────
PressureSensor::PressureSensor(int Pin) {  // CONSTRUCTOR
	IOPin = Pin;
}
PressureSensor::~PressureSensor() {}       // DESTRUCTOR

// ─────────────────────────────────────────────────────────────────────────────
// PressureSensor::Maintain  (called from MaintainTankFilling() every 10 ms)
// Each sensor instance uses its own TimerIdx so that TankWaterLevel and
// FilterSensor tick independently without sharing a timer.
// Maintains a 15-point circular buffer of raw ADC readings:
//   - On first call (Idx == -1): pre-fills the entire buffer so the average
//     is valid from the very first reading, avoiding a ramp-up period.
//   - On subsequent calls: writes one new ADC sample into the next buffer slot,
//     computes the rolling average, and updates StrFilterKPa for display.
// Sensor range: 0–RawValueMax → 0–103.4 kPa (0–15 PSI gauge range).
// When the filter pump relay is LOW, StrFilterKPa is forced to "0 kPa"
// regardless of the raw ADC reading (tank back-pressure is ignored).
// Uses digitalRead() — OUTStat is not updated by all relay control paths.
//
// FilterPressureExceeded latches only after FILTER_EXCEED_TICKS consecutive
// ticks above BackwashWarning (≈ 5 seconds), rejecting momentary spikes.
// pressureExceedCount resets to 0 whenever pressure drops below the threshold.
// ─────────────────────────────────────────────────────────────────────────────
void PressureSensor::Maintain(){
	if(TimerExpired(TimerIdx)){
		activateTimer(TimerIdx);

		// Pre-fill buffer on first call so average is valid immediately
		if(Idx == -1){
			Idx = 0;
			for(int i = 0; i < TANK_AVE_OF; i++){
				AveList[i] = analogRead(IOPin);
			}
		}
		// Update one slot in the circular buffer
		AveList[Idx] = analogRead(IOPin);
		Idx++; if(Idx >= TANK_AVE_OF) Idx = 0;

		// Compute rolling average across all buffer entries
		RawValue = 0;
		for(int i = 0; i < TANK_AVE_OF; i++){
			RawValue += AveList[i];
		}
		RawValue = RawValue / TANK_AVE_OF;
		StrFilterRawValue = String(RawValue);
		if(!digitalRead(OutPort[O_TANK_FILTER_PUMP_RELAY].Port)) {
			StrFilterKPa = "0 kPa";  // Filter off — suppress tank back-pressure
		} else if(RawValueMax > 0) {
			StrFilterKPa = String((int)((float)RawValue / (float)RawValueMax * 103.4f)) + "kPa";
		}

		// Latch FilterPressureExceeded only after FILTER_EXCEED_TICKS consecutive
		// ticks above BackwashWarning (17 × 300 ms ≈ 5 s), so transient spikes
		// are ignored.  Counter resets to zero the moment pressure drops below
		// the threshold, requiring a full unbroken 5-second window to trigger.
		// NTFY alert is sent at NtfyMessageTime by MaintainNTFY() (16_NTFY.cpp)
		int kPa = getValue();
		if (kPa > BackwashWarning) {
			if (!FilterPressureExceeded) {
				pressureExceedCount++;
				if (pressureExceedCount >= FILTER_EXCEED_TICKS) {
					FilterPressureExceeded = true;
					WriteData();
					SaveGenReport("Filter Pressure Exceeded: " + String(kPa) + " kPa (Limit: " + String(BackwashWarning) + " kPa)");
				}
			}
		} else {
			pressureExceedCount = 0;   // pressure dropped — reset debounce counter
		}
	}
}

// -----------------------------------------------------------------------------
// PressureSensor::getValue
// Returns 0 when the filter pump is not running (tank back-pressure ignored).
// When running, maps RawValue linearly from 0–RawValueMax to 0–103.4 kPa.
// Returns 0 if RawValueMax is not calibrated (avoids divide-by-zero).
// -----------------------------------------------------------------------------
int PressureSensor::getValue(){
	if(!digitalRead(OutPort[O_TANK_FILTER_PUMP_RELAY].Port)) return 0;
	if(RawValueMax <= 0) return 0;
	return (int)((float)RawValue / (float)RawValueMax * 103.4f);
}

// -----------------------------------------------------------------------------
// PressureSensor::WriteData
// Serialises calibration parameters to /FilterSensorData.json.
// -----------------------------------------------------------------------------
void PressureSensor::WriteData(){
	String StrFileName = "/FilterSensorData.json";
	File ChannelDataFile = SPIFFS.open(StrFileName, "w");
	JsonDocument doc;

	doc["RawValueMax"]              = RawValueMax;
	doc["BackwashWarning"]          = BackwashWarning;
	doc["FilterPressureExceeded"]   = FilterPressureExceeded;

	if (ChannelDataFile)
	{
		serializeJsonPretty(doc, ChannelDataFile);
		ChannelDataFile.close();
	}
}

// -----------------------------------------------------------------------------
// PressureSensor::ReadData
// Deserialises /FilterSensorData.json.  Missing or "null" keys leave the
// inline defaults in place.
// -----------------------------------------------------------------------------
void PressureSensor::ReadData(){
	String StrFileName = "/FilterSensorData.json";
	File ChannelDataFile = SPIFFS.open(StrFileName, "r");
	if (ChannelDataFile)
	{
		size_t size = ChannelDataFile.size();
		if (size <= 256)
		{
			std::unique_ptr<char[]> buf(new char[size]);
			ChannelDataFile.readBytes(buf.get(), size);
			JsonDocument doc;
			auto error = deserializeJson(doc, buf.get());
			if (!error)
			{
				if ( doc["RawValueMax"].as<String>()             != "null" ) RawValueMax             = doc["RawValueMax"];
				if ( doc["BackwashWarning"].as<String>()         != "null" ) BackwashWarning         = doc["BackwashWarning"];
				if ( doc["FilterPressureExceeded"].as<String>()  != "null" ) FilterPressureExceeded  = doc["FilterPressureExceeded"].as<bool>();
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrFileName);
			}
		}
		else
		{
			DEBUG_PRINT(StrFileName);  DEBUG_PRINTLN(" File size is > 256 ");
		}
		ChannelDataFile.close();
	}
}


