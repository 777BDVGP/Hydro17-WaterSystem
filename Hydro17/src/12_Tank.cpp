// =============================================================================
// 12_Tank.cpp — Tank Level Sensor, Fill Control & Tank Page Handler
// =============================================================================
// Implements the TankLevel class (JSON persistence), the /tank page handler,
// and MaintainTankFilling() — the main state machine that controls the tank
// fill solenoid, borehole pump, and filter pump relay based on the configured
// sensing mode (float switch or pressure sensor), fill thresholds, and the
// optional scheduled fill window.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// TankPageSel
// Builds the tab-strip for the Tank page.  Mirrors appendMainPageSel() but
// appends a START/STOP FILL TANK action link when TankFillingSense is
// "Level Sensor".  STOP is shown only while filling is active; it is greyed
// out when the current level is below StartFillLevel because the auto-logic
// would immediately restart filling in that state.
// ─────────────────────────────────────────────────────────────────────────────
void TankPageSel(int Index, AsyncResponseStream* r) {
	r->print(F("<div class='nav'>"));
	r->print((Index == PS_HOME)       ? "<a class='act'>HOME</a>"        : "<a href='/'>HOME</a>");
	r->print((Index == PS_IRRIGATION) ? "<a class='act'>IRRIGATION</a>"  : "<a href='/irrigation'>IRRIGATION</a>");
	r->print((Index == PS_TANK)       ? "<a class='act'>TANK</a>"        : "<a href='/tank'>TANK</a>");
	r->print((Index == PS_POOL)       ? "<a class='act'>POOL FILTER</a>" : "<a href='/pool'>POOL FILTER</a>");
	r->print((Index == PS_POOL_LIGHT) ? "<a class='act'>POOL LIGHT</a>"  : "<a href='/poollight'>POOL LIGHT</a>");
	r->print((Index == PS_OTHER)      ? "<a class='act'>OTHER</a>"       : "<a href='/other'>OTHER</a>");
	if (TankWaterLevel->TankFillingSense == TANK_FILL_SENSE_LEVEL) {
		if (LevelHigh) {
			r->print(TankWaterLevel->getValue() >= TankWaterLevel->StartFillLevel
			         ? "<a class='warn' href='/stoptankfill'>STOP FILL TANK</a>"
			         : "<a class='dis'>STOP FILL TANK</a>");
		} else {
			r->print(TankWaterLevel->getValue() < TankWaterLevel->StopFillLevel
			         ? "<a class='warn' href='/starttankfill'>START FILL TANK</a>"
			         : "<a class='dis'>START FILL TANK</a>");
		}
	}
	r->print(F("</div>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// handleTank  (GET /tank)
// Serves the Tank management page.
// ─────────────────────────────────────────────────────────────────────────────
void handleTank(AsyncWebServerRequest *request){
	//SaveGenReport("handleTank " + String(ESP.getFreeHeap()));

	CompilePage_Tank(request);
}

void CompilePage_Tank(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	TankPageSel(PS_TANK, response);
	response->print(F("<style>"
	                  "table td,table th{padding:1px 6px}"
	                  "input[type=time],input[type=text],input[type=number],input[type=date],select{width:95px;max-width:36vw}"
	                  "</style>"));
	append_Form_Tank(response);
	response->print(webScriptTag);
	WT_SETUP_Tank(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

// ─────────────────────────────────────────────────────────────────────────────
// handleStartTankFill  (GET /starttankfill)
// Manually starts tank filling regardless of the current level, as long as
// TankFillingSense == "Level Sensor".  Opens the fill solenoid directly and
// sets LevelHigh=true so the auto-logic knows filling is in progress and
// will not immediately re-evaluate the start threshold.
// ─────────────────────────────────────────────────────────────────────────────
void handleStartTankFill(AsyncWebServerRequest *request) {
	//SaveGenReport("handleStartTankFill " + String(ESP.getFreeHeap()));
	if (TankWaterLevel->TankFillingSense == TANK_FILL_SENSE_LEVEL &&
	    TankWaterLevel->getValue() < TankWaterLevel->StopFillLevel) {
		digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, HIGH);
		ActivateValves(CONFIG_10, STATE_OPEN_REQ);
		LevelHigh      = true;
		TankIsFull     = false;
		TankFillChange = false;
		ActivateTankFilterTimer();
		SaveGenReport("Tank fill manually started via web");
	}
	CompilePage_Tank(request);
}

// ─────────────────────────────────────────────────────────────────────────────
// handleStopTankFill  (GET /stoptankfill)
// Manually stops tank filling before StopFillLevel is reached.  Only allowed
// when the current level is >= StartFillLevel; below that the auto-logic would
// immediately reopen the solenoid on the next 10 ms cycle anyway.
// ─────────────────────────────────────────────────────────────────────────────
void handleStopTankFill(AsyncWebServerRequest *request) {
	//SaveGenReport("handleStopTankFill " + String(ESP.getFreeHeap()));
	if (TankWaterLevel->TankFillingSense == TANK_FILL_SENSE_LEVEL &&
	    TankWaterLevel->getValue() >= TankWaterLevel->StartFillLevel) {
		ActivateValves(CONFIG_10, STATE_CLOSE_REQ);
		LevelHigh      = false;
		TankIsFull     = true;
		TankFillChange = false;
		ActivateTankFilterTimer();
		SaveGenReport("Tank fill manually stopped via web");
	}
	CompilePage_Tank(request);
}

void append_Form_Tank(AsyncResponseStream* response)
{
} 

void MapTankPage(){	 // MapSettingsPage()
//  In this routine a list, TankList, is setup to connect the Webpage and the Settings Variables
	TankPageObject = true;

	TankList = new PageVar[PAGE_VAR_TANK];
	for (int i=0; i < PAGE_VAR_TANK; i++)
	{
		switch(i)
		{
			case TANK_RAW_VALUE: // Display only
				TankList[i].Var = new PageElement(&TankWaterLevel->StrTankRawValue, F("Tank Raw Value:"), "", &StrTankRawValueAtt, i+ID_TANK);
				break;
			case TANK_SENSOR_100:
				TankList[i].Var = new PageElement(&TankWaterLevel->RawValueMax, F("Tank Sensor 100% Value"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_SENSOR_100_FILTER:
				TankList[i].Var = new PageElement(&TankWaterLevel->RawValueMaxFil, F("Tank Sensor 100% Filter On"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_SENSOR_0:
				TankList[i].Var = new PageElement(&TankWaterLevel->RawValueMin, F("Tank Sensor 0% Value"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILL_SENSE:
				TankList[i].Var = new PageElement(&TankWaterLevel->TankFillingSense, F("Tank Filling Sensor"), "", &SetStyle, i+ID_TANK);
				TankList[i].Var->SetStringToSelect(2);
				TankList[i].Var->AddOption(TANK_FILL_SENSE_FLOAT);
				TankList[i].Var->AddOption(TANK_FILL_SENSE_LEVEL);
				break;
			case TANK_LEVEL_START_FILL:
				TankList[i].Var = new PageElement(&TankWaterLevel->StartFillLevel, F("Start % for Level Sensor"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_LEVEL_STOP_FILL:
				TankList[i].Var = new PageElement(&TankWaterLevel->StopFillLevel, F("Stop % for Level Sensor"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILL_OPER:
				TankList[i].Var = new PageElement(&TankWaterLevel->TankFillingTime, F("Tank Filling Time"), "", &SetStyle, i+ID_TANK);
				TankList[i].Var->SetStringToSelect(2);
				TankList[i].Var->AddOption(TANK_FILL_OPER_NORMAL);
				TankList[i].Var->AddOption(TANK_FILL_OPER_SHEDULE);
				break;
			case TANK_SHED_START:
				TankList[i].Var = new PageElement(&TankWaterLevel->TankFillStartTime, F("Start for Scheduled Time"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_SHED_STOP:
				TankList[i].Var = new PageElement(&TankWaterLevel->TankFillStopTime, F("Scheduled Time Period"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILTPUMP:
				TankList[i].Var = new PageElement(&TankWaterLevel->FilterPumpDelay, F("Standard Filtration Runtime (min)"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILTPUMP_RED_FROM:
				TankList[i].Var = new PageElement(&TankWaterLevel->ReducedPumpFromTime, F("Eco Schedule / Eco Window"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILTPUMP_RED_TO:
				TankList[i].Var = new PageElement(&TankWaterLevel->ReducedPumpToTime, F(""), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILTPUMP_RED_DELAY:
				TankList[i].Var = new PageElement(&TankWaterLevel->ReducedFilterPumpDelay, F("Eco Filtration Runtime (min)"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_KPA_VALUE:  // Display only
				TankList[i].Var = new PageElement(&FilterSensor->StrFilterKPa, F("Filter Pressure:"), "", &StrFilterPressureAtt, i+ID_TANK);
				break;
			case TANK_BACKWASH:
				TankList[i].Var = new PageElement(&FilterSensor->BackwashWarning, F("Filter BW Warning (kPa)"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_SENSOR_MAX:
				TankList[i].Var = new PageElement(&FilterSensor->RawValueMax, F("Filter Sensor Max"), "", &SetStyle, i+ID_TANK);
				break;
			case TANK_FILTER_RAW_VALUE:  // Display only
				TankList[i].Var = new PageElement(&FilterSensor->StrFilterRawValue, F("Filter Raw Value:"), "", &StrFilterRawValueAtt, i+ID_TANK);
				break;
			case TANK_FILTER_EXCEEDED:
				TankList[i].Var = new PageElement(&FilterSensor->FilterPressureExceeded, F("Filter Pressure Exceeded"), "", &SetStyle, i+ID_TANK);
				break;
		}
	}
}

void WT_SETUP_Tank(AsyncResponseStream* response){
// 	Create List of Setting Page Parameters
	resetPageBuf();
	WT_StartPage();
 	WT_StartTable("Tank Parameters", "Value", "");

	// This is Inherited from 08_Home.ino, 08_Home do the realtime update
	WT_DisplayRow(&HomeList[HOME_TANKLEVEL]);

	// Since this is a Display Only, TankList[TANK_RAW_VALUE], Added outside the FOR loop becauce of GetID("V")
	WT_DisplayRow(&TankList[TANK_RAW_VALUE]);

	for (int i=TANK_SENSOR_100; i <= TANK_FILTPUMP_RED_DELAY; i++)
	{
		if(i == TANK_SHED_START) continue;   // merged into single "Scheduled Time Period" row
		if(i == TANK_SHED_STOP) {
			// ── Combined "Scheduled Time Period" row: [start time] → [stop time] ──
			appendToPageBuf("{grouped:true,labels:[{");
			WT_TypeCell("text");
			WT_TextLabel(TankList[TANK_SHED_STOP].Var->GetDescription().c_str());
			WT_IDCell(TankList[TANK_SHED_STOP].Var->GetID("L").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_SHED_STOP].Var->GetFStyle().c_str()); WT_AttrsEnd();
			appendToPageBuf("}],inputs:[{");
			WT_TypeCell("time");
			WT_ValueInput(TankList[TANK_SHED_START].Var->GetName().c_str());
			WT_IDCell(TankList[TANK_SHED_START].Var->GetID("").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_SHED_START].Var->GetFStyle().c_str()); WT_AttrsEnd();
			WT_NextColumn();
			WT_TypeCell("time");
			WT_ValueInput(TankList[TANK_SHED_STOP].Var->GetName().c_str());
			WT_IDCell(TankList[TANK_SHED_STOP].Var->GetID("").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_SHED_STOP].Var->GetFStyle().c_str()); WT_AttrsEnd();
			appendToPageBuf("}],},");
		} else if(i == TANK_FILTPUMP_RED_TO || i == TANK_FILTPUMP_RED_DELAY) {
			continue;   // merged into TANK_FILTPUMP_RED_FROM block below
		} else if(i == TANK_FILTPUMP_RED_FROM) {
			// ── Row 1: "Reduced Filter Pump Period (Min)" | [10] ──────────────
			WT_EditRow(&TankList[TANK_FILTPUMP_RED_DELAY]);
			// ── Row 2: "Reduced Filter Time" | [18:00] [06:00] ──────────────
			appendToPageBuf("{grouped:true,labels:[{");
			WT_TypeCell("text");
			WT_TextLabel(TankList[TANK_FILTPUMP_RED_FROM].Var->GetDescription().c_str());
			WT_IDCell(TankList[TANK_FILTPUMP_RED_FROM].Var->GetID("L").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_FILTPUMP_RED_FROM].Var->GetFStyle().c_str()); WT_AttrsEnd();
			appendToPageBuf("}],inputs:[{");
			WT_TypeCell("time");
			WT_ValueInput(TankList[TANK_FILTPUMP_RED_FROM].Var->GetName().c_str());
			WT_IDCell(TankList[TANK_FILTPUMP_RED_FROM].Var->GetID("").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_FILTPUMP_RED_FROM].Var->GetFStyle().c_str()); WT_AttrsEnd();
			WT_NextColumn();
			WT_TypeCell("time");
			WT_ValueInput(TankList[TANK_FILTPUMP_RED_TO].Var->GetName().c_str());
			WT_IDCell(TankList[TANK_FILTPUMP_RED_TO].Var->GetID("").c_str());
			WT_AttrsStart(); WT_Attr("style", TankList[TANK_FILTPUMP_RED_TO].Var->GetFStyle().c_str()); WT_AttrsEnd();
			appendToPageBuf("}],},");
		} else {
			WT_EditRow(&TankList[i]);
		}
	}

	// ── Filter pressure live display (display only — uses "V" ID suffix) ────
	WT_DisplayRow(&TankList[TANK_KPA_VALUE]);

	// ── Filter raw value (display only — just below filter pressure) ──────────
	WT_DisplayRow(&TankList[TANK_FILTER_RAW_VALUE]);

	// ── Backwash, Sensor Max, Filter Exceeded ────────────────────────────────
	for (int i=TANK_BACKWASH; i <= TANK_FILTER_EXCEEDED; i++)
	{
		if(i == TANK_FILTER_RAW_VALUE) continue;
		WT_EditRow(&TankList[i]);
	}

	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

void MaintainTankPageValues(){

	// ── Update filter pressure attribute — same logic as MaintainHomePage ────
	// Must run here so the attribute is current before the display check below.
	// MaintainHomePage() (which normally updates StrFilterPressureAtt) is called
	// AFTER MaintainTankPageValues() in the MaintainWebService cycle, so without this
	// the colour sent by the tank page is always one cycle behind.
	if(FilterSensor->FilterPressureExceeded) {
		StrFilterPressureAtt = ATT_RED;
	} else if(FilterSensor->getValue() > FilterSensor->BackwashWarning * 0.8) {
		StrFilterPressureAtt = ATT_ORANGE;
	} else {
		StrFilterPressureAtt = ATT_GREEN;
	}

	// Because this is a Display Only, TankList[TANK_RAW_VALUE], Added ouside the FOR loop
	if(TankList[TANK_RAW_VALUE].Var->GetName() != TankList[TANK_RAW_VALUE].Var->GetNameLast())
	{
		webParamUpdate(TankList[TANK_RAW_VALUE].Var->GetName(),TankList[TANK_RAW_VALUE].Var->GetID("V"),TankList[TANK_RAW_VALUE].Var->GetVarStyle());
		TankList[TANK_RAW_VALUE].Var->UpdateLast();
	}

	// Filter pressure display only — trigger on value OR style change
	static String lastKPaStyle = "";
	bool kpaStyleChanged = (lastKPaStyle != StrFilterPressureAtt);
	if(TankList[TANK_KPA_VALUE].Var->GetName() != TankList[TANK_KPA_VALUE].Var->GetNameLast() || kpaStyleChanged)
	{
		webParamUpdate(TankList[TANK_KPA_VALUE].Var->GetName(),TankList[TANK_KPA_VALUE].Var->GetID("V"),TankList[TANK_KPA_VALUE].Var->GetVarStyle());
		TankList[TANK_KPA_VALUE].Var->UpdateLast();
		lastKPaStyle = StrFilterPressureAtt;
	}

	// Filter raw value display only
	if(TankList[TANK_FILTER_RAW_VALUE].Var->GetName() != TankList[TANK_FILTER_RAW_VALUE].Var->GetNameLast())
	{
		webParamUpdate(TankList[TANK_FILTER_RAW_VALUE].Var->GetName(),TankList[TANK_FILTER_RAW_VALUE].Var->GetID("V"),TankList[TANK_FILTER_RAW_VALUE].Var->GetVarStyle());
		TankList[TANK_FILTER_RAW_VALUE].Var->UpdateLast();
	}

	for (int i=TANK_SENSOR_100; i < PAGE_VAR_TANK; i++)
	{
		if(i == TANK_KPA_VALUE || i == TANK_FILTER_RAW_VALUE) continue; // Skip display-only fields already handled above
		if(TankList[i].Var->GetName() != TankList[i].Var->GetNameLast())
		{
			webParamUpdate(TankList[i].Var->GetName(),TankList[i].Var->GetID(""),TankList[i].Var->GetVarStyle());
			TankList[i].Var->UpdateLast();
		}
	}
}

void UpdateTankPageValues(uint32_t num, JsonObject *root){
	for (JsonPair kv : *root){	
 		//Serial.print("Num: ");Serial.print(num); 		
 		//Serial.print(" Key: ");Serial.print(kv.key().c_str()); 
 		//Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 	
		for (int i=0; i < PAGE_VAR_TANK; i++)
		{ 
			if((TankList[i].Var->GetID("") == kv.key().c_str()))				
			{	
				TankList[i].Var->SetName(String(kv.value().as<const char*>()));	 
			}	
		}	
	}
	WebMessUpdate(num, "Tank Parameters Updated!", "color: red", true);
	TankWaterLevel->WriteData();
	FilterSensor->WriteData();
	UpdateTimerTimes();
 }

/////////////////////////////////////////////////////////////////////////////////////////
void CreatePressureSensors() {
	TankWaterLevel = new TankLevel(GPIO_01);
	TankWaterLevel->TimerIdx = TIM_TANK_SENSOR;    // 500 ms — tank level averaging
	TankWaterLevel->ReadData();

	FilterSensor = new PressureSensor(GPIO_03);
	FilterSensor->TimerIdx = TIM_PRESSURE_SENSOR;  // 300 ms — filter pressure averaging
	FilterSensor->ReadData();
}

// Class TankLevel
TankLevel::TankLevel(int Pin) : PressureSensor(Pin) { // CONSTRUCTOR
}

TankLevel::~TankLevel() {}                           // DESTRUCTOR

void TankLevel::WriteData(){
	String StrFileName = "/TankLevelData.json";
	File ChannelDataFile = SPIFFS.open(StrFileName, "w");
	JsonDocument doc;

	doc["RawValueMax"] = RawValueMax;
	doc["RawValueMaxFil"] = RawValueMaxFil;
	doc["RawValueMin"] = RawValueMin;
	doc["StartFillLevel"] = StartFillLevel;
	doc["StopFillLevel"] = StopFillLevel;
	doc["FilterPumpDelay"] = FilterPumpDelay;
	doc["TankFillingSense"] = TankFillingSense;
	doc["TankFillingTime"] = TankFillingTime;
	doc["TankFillStartTime"] = TankFillStartTime;
	doc["TankFillStopTime"] = TankFillStopTime;
	doc["ReducedPumpFromTime"] = ReducedPumpFromTime;
	doc["ReducedPumpToTime"] = ReducedPumpToTime;
	doc["ReducedFilterPumpDelay"] = ReducedFilterPumpDelay;

	if (ChannelDataFile)
	{
		serializeJsonPretty(doc, ChannelDataFile);
		ChannelDataFile.close();
	}
}

void TankLevel::ReadData(){   
	String StrFileName = "/TankLevelData.json";	
	File ChannelDataFile = SPIFFS.open(StrFileName, "r");	
	if (ChannelDataFile) 
	{ 	
		size_t size = ChannelDataFile.size();    
		if (size <= 1024) 
		{
			std::unique_ptr<char[]> buf(new char[size]); // Buffer to store file contents
			ChannelDataFile.readBytes(buf.get(), size);
			JsonDocument doc; 
			auto error = deserializeJson(doc, buf.get());	
			if (!error) 
			{		
				if ( doc["RawValueMax"].as<String>() != "null" ) RawValueMax = doc["RawValueMax"];	 
				if ( doc["RawValueMaxFil"].as<String>() != "null" ) RawValueMaxFil = doc["RawValueMaxFil"];	 				
				if ( doc["RawValueMin"].as<String>() != "null" ) RawValueMin = doc["RawValueMin"];	 
				if ( doc["StartFillLevel"].as<String>() != "null" ) StartFillLevel = doc["StartFillLevel"];	 	
				if ( doc["StopFillLevel"].as<String>() != "null" ) StopFillLevel = doc["StopFillLevel"];	
				if ( doc["FilterPumpDelay"].as<String>() != "null" ) { FilterPumpDelay = doc["FilterPumpDelay"]; }					
				if ( doc["TankFillingSense"].as<String>() != "null" ){ TankFillingSense = doc["TankFillingSense"].as<String>();}
				if ( doc["TankFillingTime"].as<String>() != "null" ){ TankFillingTime = doc["TankFillingTime"].as<String>();}
				if ( doc["TankFillStartTime"].as<String>() != "null" ){ TankFillStartTime = doc["TankFillStartTime"].as<String>();}
				if ( doc["TankFillStopTime"].as<String>() != "null" ){ TankFillStopTime = doc["TankFillStopTime"].as<String>();}
				if ( doc["ReducedPumpFromTime"].as<String>() != "null" ){ ReducedPumpFromTime = doc["ReducedPumpFromTime"].as<String>();}
				if ( doc["ReducedPumpToTime"].as<String>() != "null" ){ ReducedPumpToTime = doc["ReducedPumpToTime"].as<String>();}
				if ( doc["ReducedFilterPumpDelay"].as<String>() != "null" ){ ReducedFilterPumpDelay = doc["ReducedFilterPumpDelay"]; }
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrFileName);
			}
		}
		else
		{
			DEBUG_PRINT(StrFileName);  DEBUG_PRINTLN(" File size is > 1024 ");
		}
		ChannelDataFile.close();  	
	}	
}

// ─────────────────────────────────────────────────────────────────────────────
// TankLevel::Maintain  (called from MaintainTankFilling() every 10 ms)
// Overrides PressureSensor::Maintain() to do tank-level averaging only.
// The base-class version runs kPa and backwash-alarm logic which is wrong for
// a sensor whose getValue() returns percent — this override keeps the two
// sensor types cleanly separated.
// Maintains the same 15-point circular buffer as PressureSensor:
//   - Pre-fills the entire buffer on first call so the average is valid immediately.
//   - Updates one slot per tick and recomputes the rolling average.
//   - Stores the result in StrTankRawValue for display on the Tank page.
// ─────────────────────────────────────────────────────────────────────────────
void TankLevel::Maintain() {
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
		RawValue        = RawValue / TANK_AVE_OF;
		StrTankRawValue = String(RawValue);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// TankLevel::getValue
// Returns the tank level as a percentage (0–100).
// Dual-range: uses RawValueMax when filter is off, RawValueMaxFil when on,
// compensating for the pressure head shift caused by the running filter pump.
// ─────────────────────────────────────────────────────────────────────────────
int TankLevel::getValue() {
	if(!digitalRead(OutPort[O_TANK_FILTER_PUMP_RELAY].Port)){
 		return int(float(RawValue-RawValueMin))/((float(RawValueMax-RawValueMin))/100.0);
	} else {
		return int(float(RawValue-RawValueMin))/((float(RawValueMaxFil-RawValueMin))/100.0);
	}
}

int TankLevel::getValueNotFilter() {	
 	return int(float(RawValue-RawValueMin))/((float(RawValueMax-RawValueMin))/100.0); 
}

///////////////////////////////////////////////////////////////////////////////////////////

// ─────────────────────────────────────────────────────────────────────────────
// ActivateTankFilterTimer
// Selects the correct filter pump run duration based on the current time of
// day, writes it into TIM_TANK_ON_DELAY, then starts the timer.
//
// During the reduced window (ReducedPumpFromTime → ReducedPumpToTime, which
// may span midnight, e.g. 18:00–06:00) ReducedFilterPumpDelay minutes are
// used.  Outside that window FilterPumpDelay (normal hours) is used.
//
// String comparison of zero-padded "HH:MM" is numerically equivalent to time
// ordering, so ">=" / "<" work correctly without parsing.
//
// Call this instead of activateTimer(TIM_TANK_ON_DELAY) everywhere the filter
// pump timer is started or kept alive.
// ─────────────────────────────────────────────────────────────────────────────
void ActivateTankFilterTimer() {
	String now  = GetCurrentTime();
	String from = TankWaterLevel->ReducedPumpFromTime;   // e.g. "18:00"
	String to   = TankWaterLevel->ReducedPumpToTime;     // e.g. "06:00"
	bool inReduced;
	if (from > to) {
		// Overnight window (from > to, e.g. 18:00–06:00):
		// in period when now >= from  OR  now < to
		inReduced = (now >= from || now < to);
	} else {
		// Same-day window: in period when from <= now < to
		inReduced = (now >= from && now < to);
	}
	SetTimerValue(inReduced ? TankWaterLevel->ReducedFilterPumpDelay
	                        : TankWaterLevel->FilterPumpDelay,
	              TIM_TANK_ON_DELAY);
	activateTimer(TIM_TANK_ON_DELAY);
}

void MaintainTankFilling() { // From Main loop
	TankWaterLevel->Maintain();         // 12_Tank.ino
	FilterSensor->Maintain();           // 12_AnalogRead.cpp — filter pressure sensor	
	
	if (NormalOperation)  
	{
		// Switch LOW and HIGH Leds
		if( InPort[I_FLOAT_HIGH].Input )  // Water HIGH
		{
			digitalWrite(OutPort[O_TANK_LOW_LED].Port, LOW); 	// Switch On LED  	
			digitalWrite(OutPort[O_TANK_HIGH_LED].Port, HIGH); 	// Switch Off LED  	
		}	
		if( InPort[I_FLOAT_LOW].Input )    // Water LOW
		{
			digitalWrite(OutPort[O_TANK_LOW_LED].Port, HIGH); 	// Switch On LED  	
			digitalWrite(OutPort[O_TANK_HIGH_LED].Port, LOW); 	// Switch Off LED 		 			
		} 		

		// Indicate if tank can be filled
		if(TankWaterLevel->TankFillingTime == TANK_FILL_OPER_SHEDULE) // "Scheduled Time"
		{
			AllowFillingAtTime = true; 
			if(GetCurrentTime() < TankWaterLevel->TankFillStartTime){
				if(!digitalRead(OutPort[O_BOREHOLE].Port)) AllowFillingAtTime = false;			
			}
			if(GetCurrentTime() > TankWaterLevel->TankFillStopTime){
				if(!digitalRead(OutPort[O_BOREHOLE].Port)) AllowFillingAtTime = false;	
			} 
		}
		else  // Any Time
		{		
			AllowFillingAtTime = true;	
		}	
		
		if(AllowFillingAtTime)
		{
			if(TankWaterLevel->TankFillingSense == TANK_FILL_SENSE_FLOAT) //  "Float Switch"
			{
				if( FloatHigh != InPort[I_FLOAT_HIGH].Input )  // Water HIGH
				{					
					FloatHigh = InPort[I_FLOAT_HIGH].Input;
					TankFillChange = true;
					TankIsFull = true;					
				}	
				if( FloatLow != InPort[I_FLOAT_LOW].Input )    // Water LOW
				{ 		
					FloatLow = InPort[I_FLOAT_LOW].Input;
					TankFillChange = true;	
					TankIsFull = false;	
				}

				// Set / Rest Tank Filling
				if(TankFillAllowed && TankFillChange)
				{				
					if(InPort[I_FLOAT_HIGH].Input) // Water HIGH
					{	
						ActivateValves(CONFIG_10, STATE_CLOSE_REQ ); 		// Close Solenoids
					}					
					if(InPort[I_FLOAT_LOW].Input) // Water LOW
					{		
						digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, HIGH);
						ActivateValves(CONFIG_10, STATE_OPEN_REQ ); 	    // Open Solenoids
					}
					TankFillChange = false;					
				}
				
				if(InPort[I_FLOAT_LOW].Input)
				{	
					// Keep Timer Activated
					ActivateTankFilterTimer();  
				}				
			}
			else // "Level Sensor"
			{

				if( TankWaterLevel->getValue() >= TankWaterLevel->StopFillLevel && LevelHigh)  // Water HIGH
				{ 			
					LevelHigh = false;
					TankFillChange = true;					
					TankIsFull = true;						
				}	
				if( TankWaterLevel->getValue() < TankWaterLevel->StartFillLevel && !LevelHigh)  // Water LOW
				{						
					LevelHigh = true;				
					TankFillChange = true;		
					TankIsFull = false;	
				}					

				// Set / Rest Tank Filling
				if(TankFillAllowed && TankFillChange)
				{	
					if( TankWaterLevel->getValue() >= TankWaterLevel->StopFillLevel )  // Water HIGH
					{
						ActivateValves(CONFIG_10, STATE_CLOSE_REQ ); 		// Close Solenoids					
					}	
					if( TankWaterLevel->getValue() < TankWaterLevel->StartFillLevel )  // Water LOW
					{					
						digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, HIGH);
						ActivateValves(CONFIG_10, STATE_OPEN_REQ ); 	    // Close Solenoids
					}				
					TankFillChange = false;					
				}

				if(!TankIsFull) // Is NOT full
				{	
					// Keep Timer Activated
					ActivateTankFilterTimer();  
				}	
			}
		}
		else
		{
			if(digitalRead(OutPort[O_TANK_FILL_SOLENOID].Port)){ 
				ActivateValves(CONFIG_10, STATE_CLOSE_REQ ); 		// Close Solenoids	
				TankIsFull = true;			
			}
		}
	}
	
	// TANK FILTER ON OFF Button — blocked while borehole is running
	if(InPort[I_TANK_FILTER_ON_OFF].Input)
	{
		if(TankFilterOnOffButton) {
			TankFilterOnOffButton = false;
			if(!digitalRead(OutPort[O_BOREHOLE].Port)) {  // Only act when borehole is NOT running
				if(digitalRead(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port)) {
					deactivateTimer(TIM_TANK_ON_DELAY);
					digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, LOW);
				} else {
					ActivateTankFilterTimer();
					digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, HIGH);
				}
			}
		}
	}
	else
	{
		TankFilterOnOffButton = true;
	}
	
	// While borehole is running keep filter pump on and prevent timer from expiring
	if (digitalRead(OutPort[O_BOREHOLE].Port))
	{
		digitalWrite(OutPort[O_TANK_FILTER_PUMP_RELAY].Port, HIGH);
		ActivateTankFilterTimer();
	}

	// Turn off filter pump after delay — applies when tank is full OR borehole has stopped
	if (TankIsFull || !digitalRead(OutPort[O_BOREHOLE].Port))
	{
		if(TimerExpired(TIM_TANK_ON_DELAY)) {
			digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, LOW);
		}
	}
	
}
