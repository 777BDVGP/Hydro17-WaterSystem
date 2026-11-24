
// =============================================================================
// 11_Other.cpp — Manual Operations, Controller Enable/Disable & Report Handlers
// =============================================================================
// Implements the manual operation page (/manual) with per-output on/off buttons,
// the controller disable/enable handlers (/discontroller, /enablecontroller),
// the general/irrigation/tank-fill report page handlers, and SwitchNormalOperation()
// which transitions the system between automatic and manual-override modes.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handleManualOperContDis  (GET /discontroller)
// Switches the system to NOT-NormalOperation mode and redirects to the Options page.
// ─────────────────────────────────────────────────────────────────────────────
void handleManualOperContDis(AsyncWebServerRequest *request){
	//SaveGenReport("handleManualOperContDis " + String(ESP.getFreeHeap()));
	SwitchNormalOperation();
	handleOptions(request);
}

void handleManualOperContEna(AsyncWebServerRequest *request){
	//SaveGenReport("handleManualOperContEna " + String(ESP.getFreeHeap()));
	SwitchNormalOperation();
	handleOptions(request);
}

void handleOptions(AsyncWebServerRequest *request){
	//SaveGenReport("handleOptions " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(Delmess, response);
	append_UL_OptionsMenu(PS_OTHER, response);
	response->print(webScriptTag);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_UL_OptionsMenu(int Index, AsyncResponseStream* r)
{
	r->print(F("<div class='nav'>"));
	r->print(F("<a href='/'>HOME</a>"));
	r->print((Index == PS_OTHER)    ? "<a class='act'>OTHER</a>"    : "<a href='/other'>OTHER</a>");
	r->print((Index == PS_REPORTS)  ? "<a class='act'>REPORTS</a>"  : "<a href='/reports'>REPORTS</a>");
	r->print((Index == PS_SETTINGS) ? "<a class='act'>SETTINGS</a>" : "<a href='/settings'>SETTINGS</a>");
	r->print(F("<a href='/files'>FILES</a>"));
	r->print((Index == PS_NTFY)     ? "<a class='act'>NTFY</a>"     : "<a href='/ntfy'>NTFY</a>");
	r->print((Index == PS_ABOUT)    ? "<a class='act'>ABOUT</a>"    : "<a href='/about'>ABOUT</a>");
	r->print(NormalOperation ? "<a class='warn' href='/discontroller'>DISABLE CONTROLLER</a>"
	                         : "<a class='warn' href='/enablecontroller'>ENABLE CONTROLLER</a>");
	r->print(F("</div>"));
}

///////////////////////////////////////////////////////////////////////////////////////////

void handleReports(AsyncWebServerRequest *request){
	//SaveGenReport("handleReports " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(Delmess, response);
	append_UL_ReportsMenu(PS_REPORTS, response);
	response->print(webScriptTag);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_UL_ReportsMenu(int Index, AsyncResponseStream* r)
{
	r->print(F("<div class='nav'>"));
	r->print(F("<a href='/'>HOME</a>"));
	r->print(F("<a href='/other'>OTHER</a>"));
	r->print((Index == PS_REPORTS) ? "<a class='act'>REPORTS</a>" : "<a href='/reports'>REPORTS</a>");
	r->print(F("</div>"));
	r->print(F("<div class='nav'>"));
	r->print((Index == PS_REPORT_GEN)  ? "<a class='act'>GENERAL</a>"    : "<a class='warn' href='/reportgen'>GENERAL</a>");
	r->print((Index == PS_REPORT_PUMP) ? "<a class='act'>IRRIGATION</a>" : "<a class='warn' href='/reportpump'>IRRIGATION</a>");
	r->print((Index == PS_REPORT_FILL) ? "<a class='act'>TANKFILL</a>"   : "<a class='warn' href='/reportfill'>TANKFILL</a>");
	r->print(F("</div>"));
}


void CreateButton() {
 	ButtonObject = new ButtonSelection;	
}

// Class ButtonSelection
ButtonSelection::ButtonSelection() {
	
	Selection[BUTTON_BOREHOLE].ButtonDescription = "BOREHOLE";
	Selection[BUTTON_VALVE_1].ButtonDescription = "BUITE HEK VOOR";
	Selection[BUTTON_VALVE_2].ButtonDescription = "AGTER WASKAMER";
	Selection[BUTTON_VALVE_3].ButtonDescription = "BEDDINGS/ANDER";
	Selection[BUTTON_VALVE_4].ButtonDescription = "VOOR BINNE";
	Selection[BUTTON_VALVE_5].ButtonDescription = "SYPAD ONDER";
	Selection[BUTTON_VALVE_6].ButtonDescription = "SYPAD BO";
	Selection[BUTTON_SUPPLY_TANK_SOL].ButtonDescription = "SUPPLY TANK VALVE";	
	Selection[BUTTON_TANK_FILL_SOL].ButtonDescription = "TANK FILTER VALVE";
	Selection[BUTTON_TANK_FILTER_PUMP_RELAY].ButtonDescription = "TANK FILTER PUMP";		
	Selection[BUTTON_POOL_LIGHT_RELAY].ButtonDescription = "POOL LIGHT";	
	Selection[BUTTON_POOL_PUMP_RELAY].ButtonDescription = "POOL PUMP";		
	Selection[BUTTON_PLUGS_SWITCH_RELAY].ButtonDescription = "PLUGS";	
	
	Selection[BUTTON_BOREHOLE].OutputPin = O_BOREHOLE;
	Selection[BUTTON_VALVE_1].OutputPin = O_VALVE_SOL_1;	
	Selection[BUTTON_VALVE_2].OutputPin = O_VALVE_SOL_2;
	Selection[BUTTON_VALVE_3].OutputPin = O_VALVE_SOL_3;		
	Selection[BUTTON_VALVE_4].OutputPin = O_VALVE_SOL_4;
	Selection[BUTTON_VALVE_5].OutputPin = O_VALVE_SOL_5;	
	Selection[BUTTON_VALVE_6].OutputPin = O_VALVE_SOL_6;
	Selection[BUTTON_SUPPLY_TANK_SOL].OutputPin = O_TANK_SUPPLY_SOLENOID;	
	Selection[BUTTON_TANK_FILL_SOL].OutputPin = O_TANK_FILL_SOLENOID;		
 	Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin = O_TANK_FILTER_PUMP_RELAY; 
 	Selection[BUTTON_POOL_LIGHT_RELAY].OutputPin = O_POOL_LIGHT_RELAY;        
 	Selection[BUTTON_POOL_PUMP_RELAY].OutputPin = O_POOL_PUMP_RELAY;	      
 	Selection[BUTTON_PLUGS_SWITCH_RELAY].OutputPin = O_PLUGS_SWITCH_RELAY;    
      
}  
 
ButtonSelection::~ButtonSelection() {}  // DESTRUCTOR

void InitFor_NormalOperation() {
	
	SaveGenReport F("NormalOperation ENABLED");	
	
	for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
	{				
		SheduleObject->IrrigateShedule[x].ManualStop = false;					
		SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_DEACTIVE;
		ActivateValves(IrriObject->Selection[x].ConfigLine.substring(7,8).toInt()-1, STATE_CLOSE_REQ);
	}

	TankFillAllowed = true;	
	TankFilterOnOffButton = true;
 	TankFillChange = false;
	TankIsFull = true;		
 	FloatHigh = false;  
 	FloatLow = false; 
	MaintainValveStatus(true);
//	MaintainTankFilling();
	
	SUSPEND_IRRI = CurrentSUSPEND_IRRI;
	ForceHomeIrrigatePush();
}

void InitFor_NOT_NormalOperation() {
	
	SaveGenReport F("NormalOperation DISABLED");	
	
	CurrentSUSPEND_IRRI = SUSPEND_IRRI;  // save before SuspendAllIrrigation changes anything
	// Susspend All Irrigations
	SuspendAllIrrigation();
	SUSPEND_IRRI = true; // Switch OFF irrigation

	// Switch Off Float Ligths
	digitalWrite(OutPort[O_TANK_LOW_LED].Port, LOW); 	 
	digitalWrite(OutPort[O_TANK_HIGH_LED].Port, LOW); 	 		

    // All Solenoids OFF
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_BOREHOLE].OutputPin)].Port, LOW); 
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_1].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_2].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_3].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_4].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_5].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_VALVE_6].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_SUPPLY_TANK_SOL].OutputPin)].Port, LOW);  
    digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILL_SOL].OutputPin)].Port, LOW);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_TANK_FILTER_PUMP_RELAY].OutputPin)].Port, LOW);
}

void MaintainController() { // From Main loop
	
	// CONTROLLER DISABLE ENABLE Button		
	if(InPort[I_SYSTEM_DISABLE_ENABLE].Input) 
	{		
		if(ControllerDisableEnable){
			ControllerDisableEnable = false;
			SwitchNormalOperation();
		}	
	}
	else
	{
		ControllerDisableEnable = true;
	}
}

void SwitchNormalOperation(){
	activateTimer(TIM_ENABLE_NORMAL_OPERATION);  // pre-arm before NormalOperation changes to close MaintainTimers race
	NormalOperation = !NormalOperation;
	digitalWrite(OutPort[O_SYSTEM_DISA_ENA_LED].Port, !NormalOperation); // LED = Inverse of NormalOperation
	if(NormalOperation) { InitFor_NormalOperation();} else { InitFor_NOT_NormalOperation();}	

    Params->NormalOperationSaved = NormalOperation;
	Params->WriteData();
}

void SetMessColour(String Mess, String Color) 
{
	ACTION_INST = Mess;
	COLOR = Color;  
}

/* handleGenReport - START */
// ─────────────────────────────────────────────────────────────────────────────
// CompilePage_ViewGeneralReport  (GET /reportgen)
// Shows the last REPORT_LINES entries from GeneralLog.txt with full HTML
// navigation.  Older entries beyond REPORT_LINES are counted and noted at the
// top of the list.  The hard cap keeps the page String small enough to build
// without exhausting heap regardless of total log size.
// ─────────────────────────────────────────────────────────────────────────────
void handleGenReport(AsyncWebServerRequest *request)
{
	//SaveGenReport("handleGenReport " + String(ESP.getFreeHeap()));
	SetMessColour("View GeneralLog Report (Last " + String(REPORT_LINES) + ")", COLOR_DEFAULT);
	CompilePage_ViewGeneralReport(request);
}

void CompilePage_ViewGeneralReport(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_ReportsMenu(PS_REPORT_GEN, response);
	response->print(webScriptTag);
	append_Form_ViewGeneralReport(response);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_ViewGeneralReport(AsyncResponseStream* response)
{
	int Records = 0;
	bool Flag = true;

	response->print(F("<form> <a> </a><br><b>GeneralLog.txt</b><br>"));
	f = SPIFFS.open(StrGeneralLog, "r");
	if (!f) { response->print(F("<a>File GeneralLog.txt not available!</a><br>")); return; }
	while (f.available()) { Records++; line = f.readStringUntil('\n'); }
	f.close();

	Records -= REPORT_LINES;

	f = SPIFFS.open(StrGeneralLog, "r");
	if (!f) { response->print(F("<a>File GeneralLog.txt not available!</a><br>")); return; }
	while (f.available()) {
		line = f.readStringUntil('\n');
		if (Records < 0) {
			response->print(F("<p>")); response->print(line); response->print(F("</p>"));
		} else if (Flag) {
			Flag = false;
			response->print(F("<p>Older records not shown: ")); response->print(String(Records)); response->print(F("</p>"));
		}
		Records--;
	}
	f.close();
	response->print(F("</form>"));
}
/* handleGenReport - END */ 
/* SaveGenReport - START */
void SaveGenReport (String p_entry)  
{
	String Message = GetCurrentDateAndTime();  Message += " "; Message += p_entry;		
	File f;
	f = SPIFFS.open(StrGeneralLog, "a");
	if (!f) 
	{ 
	}
	else 
	{
		f.println(Message);
		f.close();   
	}
} 
/* SaveGenReport - END */
/* ViewIrrigReport - START */
void handleIrrigReport(AsyncWebServerRequest *request)
{
	//SaveGenReport("handleIrrigReport " + String(ESP.getFreeHeap()));
	SetMessColour("View PumpingLog Report (Last " + String(REPORT_LINES) + ")", COLOR_DEFAULT);
	CompilePage_ViewIrrigReport(request);
}

void CompilePage_ViewIrrigReport(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_ReportsMenu(PS_REPORT_PUMP, response);
	response->print(webScriptTag);
	append_Form_ViewIrrigReport(response);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_ViewIrrigReport(AsyncResponseStream* response)
{
	int Records = 0;
	bool Flag = true;

	response->print(F("<form> <a> </a><br><b>IrrigationLog.txt</b><br>"));
	f = SPIFFS.open(StrIrrigationLog, "r");
	if (!f) { response->print(F("<a>File IrrigationLog.txt not available!</a><br>")); return; }
	while (f.available()) { Records++; line = f.readStringUntil('\n'); }
	f.close();

	Records -= REPORT_LINES;

	f = SPIFFS.open(StrIrrigationLog, "r");
	if (!f) { response->print(F("<a>File IrrigationLog.txt not available!</a><br>")); return; }
	while (f.available()) {
		line = f.readStringUntil('\n');
		if (Records < 0) {
			response->print(F("<p>")); response->print(line); response->print(F("</p>"));
		} else if (Flag) {
			Flag = false;
			response->print(F("<p>Older records not shown: ")); response->print(String(Records)); response->print(F("</p>"));
		}
		Records--;
	}
	f.close();
	response->print(F("</form>"));
}
/* ViewIrrigReport - END */ 
/* SaveIrrigReport - START */
void SaveIrrigReport (String p_entry)  
{
	File f;
	f = SPIFFS.open(StrIrrigationLog, "a");
	if (!f) 
	{ 
	
	}
	else 
	{
		f.println(p_entry);
		f.close();   
	}
} 							 
/* SaveIrrigReport - END */

/* ViewTfillReport - START */
void handleTfillReport(AsyncWebServerRequest *request)
{
	//SaveGenReport("handleTfillReport " + String(ESP.getFreeHeap()));
	SetMessColour("View TankFillingLog Report (Last " + String(REPORT_LINES) + ")", COLOR_DEFAULT);
	CompilePage_ViewTfillReport(request);
}

void CompilePage_ViewTfillReport(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_ReportsMenu(PS_REPORT_FILL, response);
	response->print(webScriptTag);
	append_Form_ViewTfillReport(response);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_ViewTfillReport(AsyncResponseStream* response)
{
	int Records = 0;
	bool Flag = true;

	response->print(F("<form> <a> </a><br><b>TankFillingLog.txt</b><br>"));
	f = SPIFFS.open(StrTankFillLog, "r");
	if (!f) { response->print(F("<a>File TankFillingLog.txt not available!</a><br>")); return; }
	while (f.available()) { Records++; line = f.readStringUntil('\n'); }
	f.close();

	Records -= REPORT_LINES;

	f = SPIFFS.open(StrTankFillLog, "r");
	if (!f) { response->print(F("<a>File TankFillingLog.txt not available!</a><br>")); return; }
	while (f.available()) {
		line = f.readStringUntil('\n');
		if (Records < 0) {
			response->print(F("<p>")); response->print(line); response->print(F("</p>"));
		} else if (Flag) {
			Flag = false;
			response->print(F("<p>Older records not shown: ")); response->print(String(Records)); response->print(F("</p>"));
		}
		Records--;
	}
	f.close();
	response->print(F("</form>"));
}
/* ViewTfillReport - END */ 

/* SaveTankFillReport - START */
void SaveTankFillReport (String p_entry)  
{
		LastTankFillReport = p_entry;
		File f;
		f = SPIFFS.open(StrTankFillLog, "a"); 
		if (!f) 
		{ 
		
		}
		else 
		{
			f.println(p_entry);
			f.close();   
		}
} 							 
/* SaveTankFillReport - END */