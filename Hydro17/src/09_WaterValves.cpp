// =============================================================================
// 09_WaterValves.cpp — Electric Valve Configuration & Switching Logic
// =============================================================================
// Implements the ElectricValves class (JSON persistence), the /valvenames and
// /valveconfigs page handlers, and MaintainValveStatus() — the state machine
// that physically opens and closes solenoid valves with configurable on/off
// switch delays to protect the pump and pipework from pressure spikes.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handleWaterValves  (GET /valvenames)
// Serves the Valve Names configuration page.
// ─────────────────────────────────────────────────────────────────────────────
void handleWaterValves(AsyncWebServerRequest *request){
	//SaveGenReport("handleWaterValves " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	IrrigatePageSel(PS_IRRIGATION_VALVES, response);
	response->print(webScriptTag);
	WT_SETUP_WaterValves(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

void WT_SETUP_WaterValves(AsyncResponseStream* response){
//  Compile a Serial String of parameters to send to Web Page
	resetPageBuf();
	
	int ID = ID_VALVES;  // Start ID with ID_VALVES 	
	
	WT_StartPage();
 	WT_StartTable("Description ", "On Delay(MS) ", "Off Delay(MS) ");
	for (int i=0; i < SOL_OUTPUT_COUNT; i++)
	{
		char onBuf[8]; char offBuf[8]; char onIdBuf[8]; char offIdBuf[8];
		snprintf(onBuf,   sizeof(onBuf),   "%d", ElecValves->WaterValve[i].SwitchDelayOn);
		snprintf(offBuf,  sizeof(offBuf),  "%d", ElecValves->WaterValve[i].SwitchDelayOff);
		ElecValves->WaterValve[i].SwitchDelayOnID  = ID; snprintf(onIdBuf,  sizeof(onIdBuf),  "%d", ID); ID++;
		ElecValves->WaterValve[i].SwitchDelayOffID = ID; snprintf(offIdBuf, sizeof(offIdBuf), "%d", ID); ID++;
		WT_StartLine();
		WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text", ElecValves->WaterValve[i].Description.c_str(), "", "", nullptr, 0);
		WT_Column(WT_INPUT, WT_FIRST, WT_MORE, "number", onBuf,  onIdBuf,  "", nullptr, 0);
		WT_Column(WT_INPUT, WT_NEXT,  WT_LAST, "number", offBuf, offIdBuf, "", nullptr, 0);
		WT_EndLine();
	}		
	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

void UpdateWaterValvesPageValues(uint32_t num, JsonObject *root){
	// When the UPDATE button is pressed, all variable on the WEB Page, needs to be updated in the server.		
	for (JsonPair kv : *root){	
		for (int i=0; i < SOL_OUTPUT_COUNT; i++) // SOL_OUTPUT_COUNT / SOL_VALVE_DISPLAY
		{		
			//Serial.print("OnID Num: ");Serial.print(num); 		
			//Serial.print(" Key: ");Serial.print(kv.key().c_str());	
			//Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 
			if(ElecValves->WaterValve[i].SwitchDelayOnID == String(kv.key().c_str()).toInt()) // Search for ID
			{
				ElecValves->WaterValve[i].SwitchDelayOn = String(kv.value().as<const char*>()).toInt();	
				i = SOL_OUTPUT_COUNT; // Found - End For loop	
			}
			
			if(ElecValves->WaterValve[i].SwitchDelayOffID == String(kv.key().c_str()).toInt()) // Search for ID
			{	
				ElecValves->WaterValve[i].SwitchDelayOff = String(kv.value().as<const char*>()).toInt();	
				i = SOL_OUTPUT_COUNT;	// Found - End For loop			
			}	
		}	
	}							
	WebMessUpdate(num, "Valves Updated!", "color: red", true);	
	ElecValves->WriteData(); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////handleValvesConfigs///////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void handleValvesConfigs(AsyncWebServerRequest *request){
	//SaveGenReport("handleValvesConfigs " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	IrrigatePageSel(PS_IRRIGATION_CONFIG, response);
	response->print(webScriptTag);
	WT_SETUP_Config(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

void WT_SETUP_Config(AsyncResponseStream* response){
//  Compile a Serial String of parameters to send to Web Page
	resetPageBuf();
	
	int ID = ID_VALCONF;  // Start ID with ID_VALCONF 
 	WT_StartPage();
	WT_Header("Valve Configs");
 	for (int i=0; i < SOL_VALVE_DISPLAY; i++)
 		WT_Header(ElecValves->WaterValve[i].Description.c_str());

 	for (int i=0; i < VALVE_DISPLAY_CONFIGS; i++)
 	{
		WT_StartLine();
		WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text", ElecValves->ValveConfig[i].ConfigName.c_str(), "", "", nullptr, 0);
		int Par2 = 0; int Par3 = 0;
 		for (int j=0; j < SOL_VALVE_DISPLAY; j++)
 		{
			Par2 = (j == 0) ? WT_FIRST : WT_NEXT;
			Par3 = (j == SOL_VALVE_DISPLAY - 1) ? WT_LAST : WT_MORE;
			const char* val = ElecValves->ValveConfig[i].ValveStatus[j].State ? "true" : "false";
			char idBuf[8]; snprintf(idBuf, sizeof(idBuf), "%d", ID);
			WT_Column(WT_INPUT, Par2, Par3, "checkbox", val, idBuf, "", nullptr, 0);
			ElecValves->ValveConfig[i].ValveStatus[j].StateID = ID;
	 		ID++;
 		}
		WT_EndLine();
	}			
	WT_EndPageClass("grid");
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

void UpdateConfigPageValues(uint32_t num, JsonObject *root){
// When the UPDATE button is pressed, all variable on the WEB Page, needs to be updated in the server.		

	for (JsonPair kv : *root){	
	
 		//			Serial.print(" Num: ");Serial.print(num); 		
 		//			Serial.print(" Key: "); Serial.print(kv.key().c_str()); 
  		//			Serial.print(" Value: "); Serial.println(kv.value().as<const char*>()); 	
 
		for (int i=0; i < VALVE_DISPLAY_CONFIGS; i++)  
		{    
			for (int j=0; j < SOL_VALVE_DISPLAY; j++)  
			{ 
				if(ElecValves->ValveConfig[i].ValveStatus[j].StateID  == String(kv.key().c_str()).toInt()) // Search for ID
				{		
					if(String(kv.value().as<const char*>()) == "true" )
					{
						ElecValves->ValveConfig[i].ValveStatus[j].State = true;
					}
					else
					{
						ElecValves->ValveConfig[i].ValveStatus[j].State = false;
					}	
					i = VALVE_DISPLAY_CONFIGS; j = SOL_VALVE_DISPLAY; // Found - End For loop	
				}					
			}	
		}	
 	
	}
	WebMessUpdate(num, "Configuration Updated!", "color: red", true);	
 	ElecValves->WriteData(); 
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////Class ElectricValves////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// Class ElectricValves

// Create ElectricValves Object	
void CreateElectricValves() {
    ElecValves = new ElectricValves;	
	ElecValves->Init();
}

ElectricValves::ElectricValves() {}   // CONSTRUCTOR
ElectricValves::~ElectricValves() {}  // DESTRUCTOR

void ElectricValves::Init() {
	
	// Init the Valve IOs
	WaterValve[SOL_VALVE_1].Description  = "V1";
	WaterValve[SOL_VALVE_2].Description  = "V2";	
	WaterValve[SOL_VALVE_3].Description  = "V3";
 	WaterValve[SOL_VALVE_4].Description  = "V4";	 
 	WaterValve[SOL_VALVE_5].Description  = "V5";	
 	WaterValve[SOL_VALVE_6].Description  = "V6";
  	WaterValve[SOL_TANK_FILL].Description = "TF";	// Fank Fill	
 	WaterValve[SOL_TANK_SUP].Description  = "TS";	// Tank Supply	
  	WaterValve[SOL_BOREHOLE].Description = "BH";	// Boorgat

	WaterValve[SOL_VALVE_1].RequiredState = VALVE_CLOSE;	
	WaterValve[SOL_VALVE_2].RequiredState = VALVE_CLOSE;	
	WaterValve[SOL_VALVE_3].RequiredState = VALVE_CLOSE;		
 	WaterValve[SOL_VALVE_4].RequiredState = VALVE_CLOSE;	 
 	WaterValve[SOL_VALVE_5].RequiredState = VALVE_CLOSE;	
 	WaterValve[SOL_VALVE_6].RequiredState = VALVE_CLOSE;	
	WaterValve[SOL_TANK_FILL].RequiredState = VALVE_CLOSE;		
  	WaterValve[SOL_TANK_SUP].RequiredState = VALVE_CLOSE;	
  	WaterValve[SOL_BOREHOLE].RequiredState = VALVE_CLOSE;	
	
	WaterValve[SOL_VALVE_1].ActualState = VALVE_CLOSE;	
	WaterValve[SOL_VALVE_2].ActualState = VALVE_CLOSE;	
	WaterValve[SOL_VALVE_3].ActualState = VALVE_CLOSE;		
 	WaterValve[SOL_VALVE_4].ActualState = VALVE_CLOSE;	 
 	WaterValve[SOL_VALVE_5].ActualState = VALVE_CLOSE;	
 	WaterValve[SOL_VALVE_6].ActualState = VALVE_CLOSE;	
	WaterValve[SOL_TANK_FILL].ActualState = VALVE_CLOSE;		
  	WaterValve[SOL_TANK_SUP].ActualState = VALVE_CLOSE;	
  	WaterValve[SOL_BOREHOLE].ActualState = VALVE_CLOSE;	

	WaterValve[SOL_VALVE_1].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	
	WaterValve[SOL_VALVE_2].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	
	WaterValve[SOL_VALVE_3].SwitchDelayOn = VALVE_SWITCH_DELAY_2;		
 	WaterValve[SOL_VALVE_4].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	 
 	WaterValve[SOL_VALVE_5].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	
 	WaterValve[SOL_VALVE_6].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	
	WaterValve[SOL_TANK_FILL].SwitchDelayOn = VALVE_SWITCH_DELAY_2;		
  	WaterValve[SOL_TANK_SUP].SwitchDelayOn = VALVE_SWITCH_DELAY_2;	
  	WaterValve[SOL_BOREHOLE].SwitchDelayOn = VALVE_SWITCH_DELAY_1;	
	
	WaterValve[SOL_VALVE_1].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
	WaterValve[SOL_VALVE_2].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
	WaterValve[SOL_VALVE_3].SwitchDelayOff = VALVE_SWITCH_DELAY_1;		
 	WaterValve[SOL_VALVE_4].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	 
 	WaterValve[SOL_VALVE_5].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
 	WaterValve[SOL_VALVE_6].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
	WaterValve[SOL_TANK_FILL].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
  	WaterValve[SOL_TANK_SUP].SwitchDelayOff = VALVE_SWITCH_DELAY_1;	
  	WaterValve[SOL_BOREHOLE].SwitchDelayOff = VALVE_SWITCH_DELAY_2;	 
	
	WaterValve[SOL_VALVE_1].OutputPin   = O_VALVE_SOL_1;
	WaterValve[SOL_VALVE_2].OutputPin   = O_VALVE_SOL_2;
	WaterValve[SOL_VALVE_3].OutputPin   = O_VALVE_SOL_3;
	WaterValve[SOL_VALVE_4].OutputPin   = O_VALVE_SOL_4;
	WaterValve[SOL_VALVE_5].OutputPin   = O_VALVE_SOL_5;
	WaterValve[SOL_VALVE_6].OutputPin   = O_VALVE_SOL_6;
	WaterValve[SOL_TANK_FILL].OutputPin = O_TANK_FILL_SOLENOID;
	WaterValve[SOL_TANK_SUP].OutputPin  = O_TANK_SUPPLY_SOLENOID;
	WaterValve[SOL_BOREHOLE].OutputPin  = O_BOREHOLE;           	
	
	ValveConfig[CONFIG_1].ConfigName = CONFIG_1_STR;	
	ValveConfig[CONFIG_1].Relevance = REL_IRRILINE;	
	ValveConfig[CONFIG_2].ConfigName = CONFIG_2_STR;	
	ValveConfig[CONFIG_2].Relevance = REL_IRRILINE;	
	ValveConfig[CONFIG_3].ConfigName = CONFIG_3_STR;
	ValveConfig[CONFIG_3].Relevance = REL_IRRILINE;		
	ValveConfig[CONFIG_4].ConfigName = CONFIG_4_STR;	
	ValveConfig[CONFIG_4].Relevance = REL_IRRILINE;		
	ValveConfig[CONFIG_5].ConfigName = CONFIG_5_STR;	
	ValveConfig[CONFIG_5].Relevance = REL_IRRILINE;		
	ValveConfig[CONFIG_6].ConfigName = CONFIG_6_STR;	
	ValveConfig[CONFIG_6].Relevance = REL_IRRILINE;	 	
	ValveConfig[CONFIG_7].ConfigName = CONFIG_7_STR;
	ValveConfig[CONFIG_7].Relevance = REL_IRRILINE;			
	ValveConfig[CONFIG_8].ConfigName = CONFIG_8_STR;		
	ValveConfig[CONFIG_8].Relevance = REL_IRRILINE;		
	ValveConfig[CONFIG_9].ConfigName = CONFIG_9_STR;	
	ValveConfig[CONFIG_9].Relevance = REL_IRRILINE;

	ValveConfig[CONFIG_10].ConfigName = CONFIG_10_STR;		
	ValveConfig[CONFIG_10].Relevance = REL_TANK;		

	ElecValves->ReadData();
}

void ElectricValves::SwitchValveConfig() {
	for ( int i = 0 ; i < IRRIGATE_SELECTIONS; i++ )
	{
		// Resolve the zone's actual config index from its ConfigLine string ("Config 1" → 0, etc.)
		// Zones assigned "None" yield configIndex = -1 and are skipped.
		int configIndex = IrriObject->Selection[i].ConfigLine.substring(7, 8).toInt() - 1;
		if (configIndex < 0 || configIndex >= VALVE_DISPLAY_CONFIGS) continue;

		if(IrriObject->Selection[i].WaterSource == IRRI_SOURCE_BOLEHOLE)
		{
			ElecValves->ValveConfig[configIndex].ValveStatus[SOL_BOREHOLE].State = VALVE_ACTIVE;
			ElecValves->ValveConfig[configIndex].ValveStatus[SOL_TANK_SUP].State = VALVE_INACTIVE;
		}
		else
		{
			ElecValves->ValveConfig[configIndex].ValveStatus[SOL_BOREHOLE].State = VALVE_INACTIVE;
			ElecValves->ValveConfig[configIndex].ValveStatus[SOL_TANK_SUP].State = VALVE_ACTIVE;
		}
	}

	ElecValves->ValveConfig[CONFIG_10].ValveStatus[SOL_BOREHOLE].State = VALVE_ACTIVE;
	ElecValves->ValveConfig[CONFIG_10].ValveStatus[SOL_TANK_SUP].State = VALVE_INACTIVE;		
	ElecValves->ValveConfig[CONFIG_10].ValveStatus[SOL_TANK_FILL].State = VALVE_ACTIVE;	
	
	ElecValves->WriteData(); 

}

void ActivateValves(int Config, int ValveState ) {  // CONFIG_1 - 10 / STATE_OPEN  STATE_CLOSE
	if(Config >= 0)
	{
		if(ValveState == STATE_OPEN_REQ)
		{   
			//Serial.print("STATE_OPEN_REQ : ");  Serial.print(Config); Serial.print(" "); Serial.println(GetCurrentDateAndTime());
			IrriObject->BusyPumpingConfig = Config; 
			ElecValves->ValveConfig[Config].StartDateTime = GetCurrentDateAndTime();
			StartValveTime = millis() / 1000;
			String Desc = ElecValves->ValveConfig[Config].ConfigName;			
			if(Desc == CONFIG_10_STR) { 
				StartPerc = String(TankWaterLevel->getValueNotFilter()) + "%";
			}			
			for (int j=0; j < SOL_OUTPUT_COUNT; j++)
			{
				ElecValves->WaterValve[j].RequiredState = ElecValves->ValveConfig[Config].ValveStatus[j].State;
				ElecValves->WaterValve[j].UpdateTime = millis() + ElecValves->WaterValve[j].SwitchDelayOn;
			}
			// Borehole starting — switch on tank filter pump immediately and arm delay timer
			if (ElecValves->ValveConfig[Config].ValveStatus[SOL_BOREHOLE].State == VALVE_ACTIVE)
			{
				digitalWrite(OutPort[O_TANK_FILTER_PUMP_RELAY].Port, HIGH);
				activateTimer(TIM_TANK_ON_DELAY);
			}
		}
		else if (ValveState == STATE_CLOSE_REQ)
		{		
			//Serial.print("STATE_CLOSE_REQ : ");  Serial.print(Config); Serial.print(" "); Serial.println(GetCurrentDateAndTime());
			IrriObject->BusyPumpingConfig  = -1;
			if(ElecValves->ValveConfig[Config].StartDateTime != "") 
			{	
				String Desc = ElecValves->ValveConfig[Config].ConfigName;
				for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
				{
					if(Desc == IrriObject->Selection[x].ConfigLine)
					{
						Desc = IrriObject->Selection[x].SelectDescription;
						x = IRRIGATE_SELECTIONS;
					}
				}		
				int Duration = ( millis() / 1000 ) - StartValveTime;				
				if(Desc == CONFIG_10_STR) {
					Desc = "TANK FILLUP from "; Desc += StartPerc;
					SaveTankFillReport ( Desc + " : " +
									  ElecValves->ValveConfig[Config].StartDateTime + " for " +
									  GetHrMinSec(Duration, false) );
				}
				else
				{
					SaveIrrigReport ( Desc + " : " +
								  ElecValves->ValveConfig[Config].StartDateTime + " for " +
								  GetHrMinSec(Duration, false) );
				}
				ElecValves->ValveConfig[Config].StartDateTime = "";  // Prevent re-logging on subsequent close calls
			}
			for (int j=0; j < SOL_OUTPUT_COUNT; j++)  
			{ 
				ElecValves->WaterValve[j].RequiredState = VALVE_CLOSE;
				ElecValves->WaterValve[j].UpdateTime = millis() + ElecValves->WaterValve[j].SwitchDelayOff;		
			}	
		}
	}
}

void MaintainValveStatus(bool delay){ // From Main loop
	if (NormalOperation) {
		for (int j=0; j < SOL_OUTPUT_COUNT; j++) { 
			if(ElecValves->WaterValve[j].RequiredState == VALVE_OPEN &&
				ElecValves->WaterValve[j].ActualState == VALVE_CLOSE ) {
 				if( millis() > ElecValves->WaterValve[j].UpdateTime || delay){							
					digitalWrite(OutPort[(ElecValves->WaterValve[j].OutputPin)].Port, HIGH);
					ElecValves->WaterValve[j].ActualState = VALVE_OPEN;
					ValveLastState = VALVE_OPEN;
 				}
			}
			else if(ElecValves->WaterValve[j].RequiredState == VALVE_CLOSE &&
				ElecValves->WaterValve[j].ActualState == VALVE_OPEN ) {				
 				if( millis() > ElecValves->WaterValve[j].UpdateTime || delay){				
					digitalWrite(OutPort[(ElecValves->WaterValve[j].OutputPin)].Port, LOW);					
					ElecValves->WaterValve[j].ActualState = VALVE_CLOSE;
					ValveLastState = VALVE_CLOSE;
 				}	
			}	
		}
	}
}		

void ElectricValves::WriteData() {
	
	File ChannelDataFile = SPIFFS.open(StrElectricValves, "w");
	JsonDocument doc; 

	String docDesc;
	for (int i=0; i < SOL_OUTPUT_COUNT; i++){ 
		docDesc = F("ValveDelayOn_");  docDesc += i;
		doc[docDesc] = WaterValve[i].SwitchDelayOn; 
		docDesc = F("ValveDelayOff_");  docDesc += i;		
		doc[docDesc] = WaterValve[i].SwitchDelayOff; 		
	}
	
	int J = 0;
	for (int i=0; i < VALVE_OUTPUT_CONFIGS; i++){   
		docDesc = F("ConfRel_"); docDesc += i;
		doc[docDesc] = ValveConfig[i].Relevance;
		
		for (int j=0; j < SOL_OUTPUT_COUNT; j++){ 		
			docDesc = F("VC_"); docDesc += J; docDesc += F("C"); docDesc += i; docDesc += F("V"); docDesc += j;				
			doc[docDesc] = ValveConfig[i].ValveStatus[j].State;		
			J++;
		}	
	}	
			
	if (ChannelDataFile) 
	{   
		serializeJsonPretty(doc, ChannelDataFile);  
		ChannelDataFile.close(); 
	}	
}

void ElectricValves::ReadData() {
	
	File ChannelDataFile = SPIFFS.open(StrElectricValves, "r");	
	if (ChannelDataFile) 
	{ 	
		size_t size = ChannelDataFile.size();    
		if (size <= 6000) 
		{
			std::unique_ptr<char[]> buf(new char[size]); // Buffer to store file contents
			ChannelDataFile.readBytes(buf.get(), size);
			JsonDocument doc;
			auto error = deserializeJson(doc, buf.get());
			if (!error)
			{
				String docDesc;
				for (int i=0; i < SOL_OUTPUT_COUNT ; i++){
					docDesc = F("ValveDelayOn_");  docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) WaterValve[i].SwitchDelayOn = doc[docDesc];
					docDesc = F("ValveDelayOff_");  docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) WaterValve[i].SwitchDelayOff = doc[docDesc];
				}
				int J = 0;
				for (int i=0; i < VALVE_OUTPUT_CONFIGS; i++){
					docDesc = F("ConfRel_"); docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) ValveConfig[i].Relevance = doc[docDesc];
					
					for (int j=0; j < SOL_OUTPUT_COUNT; j++){
						docDesc = F("VC_"); docDesc += J; docDesc += F("C"); docDesc += i; docDesc += F("V"); docDesc += j;
						if ( doc[docDesc].as<String>() != "null" ) ValveConfig[i].ValveStatus[j].State = doc[docDesc];
						J++;
					}
				}
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrElectricValves);
			}
		}
		else
		{
			DEBUG_PRINT(StrElectricValves);  DEBUG_PRINTLN(" File size is > 1024 ");
		}
		ChannelDataFile.close();
	}
} 

