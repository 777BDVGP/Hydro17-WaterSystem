// =============================================================================
// 09_Irrigation.cpp — Irrigation Web Pages, Scheduling & Zone Control
// =============================================================================
// Implements all irrigation page handlers (/irrigation, /zonesnext, /zonesprev,
// /resumeirrigateion, /suspendirrigateion), the JSON-backed Irrigation class,
// the zone scheduler (IrrigationShedule), and the valve configuration logic
// that activates solenoid valve groups for each configured irrigation zone.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handleIrrigation  (GET /irrigation)
// Entry point for the irrigation zone overview page.
// ─────────────────────────────────────────────────────────────────────────────
void handleIrrigation(AsyncWebServerRequest *request){
	//SaveGenReport("handleIrrigation " + String(ESP.getFreeHeap()));
	IrrigationPage(request);
}


void IrrigationPage(AsyncWebServerRequest *request) {

	int Args = request->args();
    if ( Args == 1 )
    {
        int Line = -1;
		String paramName = request->getParam((size_t)0)->name();
		if ( paramName.substring(0,1) == "A" )
		{
			Line = paramName.substring(1,2).toInt();
			if(IrriObject->GetOption(Line) == IRRI_STATUS_SHED)
			{
				SheduleObject->IrrigateShedule[Line].ManualStop = true;
			}
			else if (IrriObject->GetOption(Line) == IRRI_STATUS_STOP)
			{
				SheduleObject->IrrigateShedule[Line].ManualStop = true;
			}
			else if (IrriObject->GetOption(Line) == IRRI_STATUS_START)
			{
				SetupIrrigateShedule(Line, SHED_TYPE_MANUAL);
			}
 			CompilePage_Irrigation(request);
		}
		else if ( paramName.substring(0,1) == "L" )
		{
			Line = paramName.substring(1,2).toInt();
			CurrentIrriSetup = Line;
			CompilePage_IrrigSetup(request, Line);
		}
	}
	else
	{
		CompilePage_Irrigation(request);
	}
}

void CompilePage_Irrigation(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	IrrigatePageSel(PS_IRRIGATION, response);
	append_Form_Irrigation(response);
	response->print(webScriptTag);
	response->print(webHTMLEnd);
	request->send(response);
}

// Page Main Variables
void IrrigatePageSel(int Index, AsyncResponseStream* r){
	r->print(F("<div class='nav'>"));
	r->print((Index == PS_HOME)               ? "<a class='act'>HOME</a>"          : "<a href='/'>HOME</a>");
	r->print((Index == PS_IRRIGATION)         ? "<a class='act'>IRRIGATION</a>"    : "<a href='/irrigation'>IRRIGATION</a>");
	r->print((Index == PS_IRRIGATION_VALVES)  ? "<a class='act'>Valve Delays</a>"  : "<a href='/valvenames'>Valve Delays</a>");
	r->print((Index == PS_IRRIGATION_CONFIG)  ? "<a class='act'>Valve Configs</a>" : "<a href='/valveconfigs'>Valve Configs</a>");
	r->print((Index == PS_IRRIGATION_SUSPEND) ? "<a class='act'>Manage Suspend</a>": "<a href='/managesuspend'>Manage Suspend</a>");
	r->print(SUSPEND_IRRI ? "<a class='warn' href='/resumeirrigateion'>RESUME IRRIGATION</a>"
	                      : "<a class='warn' href='/suspendirrigateion'>SUSPEND IRRIGATION</a>");
	r->print(F("</div>"));
}

void append_Form_Irrigation(AsyncResponseStream* response)
{
	String BUTTON = "";	String ID = "";
	response->print(F("<form><table>"));
	for ( int x = 0 ; x < IRRIGATE_SELECTIONS; x++ )
	{
		BUTTON = "L"; BUTTON += String(x);
		response->print(F("<tr>"));
		response->print(F("<th><p>"));
		response->print(F("<input type='submit' name='"));
		response->print(BUTTON);
		response->print(F("' value='"));
		response->print(IrriObject->GetDescription(x));
		response->print(F("' style='font-size:14px;' />"));
		response->print(F("</p></th>"));
		if(IrriObject->Selection[x].Status == IRRI_STATUS_ACTIVE && SUSPEND_IRRI == false)
		{
			BUTTON = "A"; BUTTON += String(x); ID = "II"; ID += String(x);
			response->print(F("<th><p>"));
			response->print(F("<input type='submit' name='"));
			response->print(BUTTON);
			response->print(F("' value='"));
			response->print(IrriObject->GetOption(x));
			response->print(F("' id='"));
			response->print(ID);
			response->print(F("' style='font-size:14px;' />"));
			response->print(F("</p></th>"));
		}
		response->print(F("</tr>"));
	}
	response->print(F("</table></form>"));
} 
 
void UpdateButtons(){
	
	String ID = "";
	for ( int x = 0 ; x < IRRIGATE_SELECTIONS; x++ )
	{
		if(IrriObject->Selection[x].Status == IRRI_STATUS_ACTIVE && SUSPEND_IRRI == false)
		{
			ID = "II"; ID += String(x); 
			webParamUpdate(IrriObject->GetOption(x),ID,"");			
		}
	}
	
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
////////handleIrrigSetup /////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////// 
 
void handleIrrigSetupNext(AsyncWebServerRequest *request){
	//SaveGenReport("handleIrrigSetupNext " + String(ESP.getFreeHeap()));

	if(CurrentIrriSetup == IRRIGATE_SEL_9)
	{
		CurrentIrriSetup = IRRIGATE_SEL_1;
	}
	else
	{
		CurrentIrriSetup++;
	}
	CompilePage_IrrigSetup(request, CurrentIrriSetup);
}

void handleIrrigSetupPrev(AsyncWebServerRequest *request){
	//SaveGenReport("handleIrrigSetupPrev " + String(ESP.getFreeHeap()));

	if(CurrentIrriSetup == IRRIGATE_SEL_1)
	{
		CurrentIrriSetup = IRRIGATE_SEL_9;
	}
	else
	{
		CurrentIrriSetup--;
	}
	CompilePage_IrrigSetup(request, CurrentIrriSetup);
}

void CompilePage_IrrigSetup(AsyncWebServerRequest *request, int Line){

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	Irrigate_Next_Prev(response);
	response->print(webScriptTag);
	WT_SETUP_IrrigSetup(response, Line);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

// Page Main Variables
void Irrigate_Next_Prev(AsyncResponseStream* r){
	r->print(F("<ul>"
	           "<li><b><a href='/'>HOME</a></b></li>"
	           "<li><b><a href='/irrigation'>IRRIGATION</a></b></li>"
	           "<li><b><a href='/zonesprev'>PREV</a></b></li>"
	           "<li><b><a href='/zonesnext'>NEXT</a></b></li>"
	           "</ul>"));
}

void WT_SETUP_IrrigSetup(AsyncResponseStream* response, int Line){
//  Compile a Serial String of parameters to send to Web Page
	resetPageBuf();
	
	int ID = ID_IRRISETUP;
	String Val = "Value Line ";  Val += String(Line + 1);

	WT_StartPage();
 	WT_StartTable("Description ", Val.c_str(), "");

	SelectOptions SuspendOpt[2];
	SuspendOpt[0].Option = SUSPEND_NO;
	SuspendOpt[1].Option = SUSPEND_YES;

	// Helper macro to emit one label+input row cleanly
	#define IRRI_ROW(label, type, valExpr, optList, optSize) \
		{ char idBuf[8]; snprintf(idBuf, sizeof(idBuf), "%d", ID); \
		  WT_StartLine(); \
		  WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text", label, "", "", nullptr, 0); \
		  WT_Column(WT_INPUT, WT_FIRST, WT_LAST, type, (valExpr), idBuf, "", optList, optSize); \
		  WT_EndLine(); }

	IrriObject->Selection[Line].SuspendALLID = ID;
	IRRI_ROW("Suspended", "select", IrriObject->Selection[Line].SuspendALL.c_str(), SuspendOpt, 2) ID++;

	IrriObject->Selection[Line].SelectDescriptionID = ID;
	IRRI_ROW("Description", "text", IrriObject->Selection[Line].SelectDescription.c_str(), nullptr, 0) ID++;

	SelectOptions OptionStatus[2];
	OptionStatus[0].Option = IRRI_STATUS_ACTIVE;
	OptionStatus[1].Option = IRRI_STATUS_INACTIVE;
	IrriObject->Selection[Line].StatusID = ID;
	IRRI_ROW("Status", "select", IrriObject->Selection[Line].Status.c_str(), OptionStatus, 2) ID++;

	int options = IRRIGATE_SELECTIONS + 1;
	SelectOptions OptionConf[options];
	for (int i = 0; i < IRRIGATE_SELECTIONS; i++) { OptionConf[i].Option = "Config " + String(i + 1); }
	OptionConf[options - 1].Option = CONFIG_NONE_STR;
	IrriObject->Selection[Line].ConfigLineID = ID;
	IRRI_ROW("Valve Config", "select", IrriObject->Selection[Line].ConfigLine.c_str(), OptionConf, options) ID++;

	SelectOptions OptionMode[2];
	OptionMode[0].Option = IRRI_MODE_MANUAL;
	OptionMode[1].Option = IRRI_MODE_SHEDULE;
	IrriObject->Selection[Line].OperModeID = ID;
	IRRI_ROW("Operation Mode", "select", IrriObject->Selection[Line].OperMode.c_str(), OptionMode, 2) ID++;

	IrriObject->Selection[Line].NextDateToIrrigateID = ID;
	IRRI_ROW("Next Date", "date", IrriObject->Selection[Line].NextDateToIrrigate.c_str(), nullptr, 0) ID++;

	IrriObject->Selection[Line].NextTimeToIrrigateID = ID;
	IRRI_ROW("Next Time", "time", IrriObject->Selection[Line].NextTimeToIrrigate.c_str(), nullptr, 0) ID++;

	IrriObject->Selection[Line].PeriodToIrrigateID = ID;
	IRRI_ROW("Irrigation Period", "time", IrriObject->Selection[Line].PeriodToIrrigate.c_str(), nullptr, 0) ID++;

	{ char freqBuf[8]; snprintf(freqBuf, sizeof(freqBuf), "%d", IrriObject->Selection[Line].IrrigationFreq);
	  IrriObject->Selection[Line].IrrigationFreqID = ID;
	  IRRI_ROW("Frequency (days)", "number", freqBuf, nullptr, 0) ID++; }

	SelectOptions OptionSource[2];
	OptionSource[0].Option = IRRI_SOURCE_TANK;
	OptionSource[1].Option = IRRI_SOURCE_BOLEHOLE;
	IrriObject->Selection[Line].WaterSourceID = ID;
	IRRI_ROW("Water Source", "select", IrriObject->Selection[Line].WaterSource.c_str(), OptionSource, 2) ID++;

	SelectOptions OptionRain[2];
	OptionRain[0].Option = IRRI_RESP_RAIN_YES;
	OptionRain[1].Option = IRRI_RESP_RAIN_NO;
	IrriObject->Selection[Line].RespectRainfailID = ID;
	IRRI_ROW("Respect Rain", "select", IrriObject->Selection[Line].RespectRainfail.c_str(), OptionRain, 2) ID++;

	IrriObject->Selection[Line].SuspendMonID = ID;
	IRRI_ROW("Susp Monday",    "select", IrriObject->Selection[Line].SuspendMon.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendTueID = ID;
	IRRI_ROW("Susp Tuesday",   "select", IrriObject->Selection[Line].SuspendTue.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendWedID = ID;
	IRRI_ROW("Susp Wednesday", "select", IrriObject->Selection[Line].SuspendWed.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendThuID = ID;
	IRRI_ROW("Susp Thursday",  "select", IrriObject->Selection[Line].SuspendThu.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendFriID = ID;
	IRRI_ROW("Susp Friday",    "select", IrriObject->Selection[Line].SuspendFri.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendSatID = ID;
	IRRI_ROW("Susp Saturday",  "select", IrriObject->Selection[Line].SuspendSat.c_str(), SuspendOpt, 2) ID++;
	IrriObject->Selection[Line].SuspendSunID = ID;
	IRRI_ROW("Susp Sunday",    "select", IrriObject->Selection[Line].SuspendSun.c_str(), SuspendOpt, 2) ID++;

	#undef IRRI_ROW
	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

void UpdateIrrigSetupPageValues(uint32_t num, JsonObject *root){
	// When the UPDATE button is pressed, all variable on the WEB Page, needs to be updated in the server.		
	for (JsonPair kv : *root){	
 	
		//Serial.print("Num: ");Serial.print(num); 		
		//Serial.print(" Key: ");Serial.print(kv.key().c_str());	
		//Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 

		if(IrriObject->Selection[CurrentIrriSetup].SelectDescriptionID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SelectDescription = String(kv.value().as<const char*>());
		}
		
		if(IrriObject->Selection[CurrentIrriSetup].StatusID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].Status = String(kv.value().as<const char*>());
		}		
		
		if(IrriObject->Selection[CurrentIrriSetup].ConfigLineID == String(kv.key().c_str()).toInt())
		{
			if(IrriObject->Selection[CurrentIrriSetup].Status == IRRI_STATUS_ACTIVE)
			{
				if(String(kv.value().as<const char*>()) == CONFIG_NONE_STR)
				{
					IrriObject->Selection[CurrentIrriSetup].Status = IRRI_STATUS_INACTIVE;
					IrriObject->Selection[CurrentIrriSetup].ConfigLine = CONFIG_NONE_STR;
				}
				else
				{
					IrriObject->Selection[CurrentIrriSetup].ConfigLine = String(kv.value().as<const char*>());
				}
			}
			else
			{
				IrriObject->Selection[CurrentIrriSetup].ConfigLine = CONFIG_NONE_STR;
			}
			// Update Page 
			webParamUpdate(IrriObject->Selection[CurrentIrriSetup].Status, String(IrriObject->Selection[CurrentIrriSetup].StatusID), "");
			webParamUpdate(IrriObject->Selection[CurrentIrriSetup].ConfigLine, String(IrriObject->Selection[CurrentIrriSetup].ConfigLineID), "");
		}		

		if(IrriObject->Selection[CurrentIrriSetup].OperModeID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].OperMode = String(kv.value().as<const char*>());
		}				
 
 		if(IrriObject->Selection[CurrentIrriSetup].NextDateToIrrigateID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].NextDateToIrrigate = String(kv.value().as<const char*>());
		}	
		
		if(IrriObject->Selection[CurrentIrriSetup].NextTimeToIrrigateID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].NextTimeToIrrigate = String(kv.value().as<const char*>());
		}	

		if(IrriObject->Selection[CurrentIrriSetup].PeriodToIrrigateID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].PeriodToIrrigate = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].IrrigationFreqID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].IrrigationFreq = String(kv.value().as<const char*>()).toInt();
		}	

		if(IrriObject->Selection[CurrentIrriSetup].WaterSourceID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].WaterSource = String(kv.value().as<const char*>());
		}	

		if(IrriObject->Selection[CurrentIrriSetup].RespectRainfailID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].RespectRainfail = String(kv.value().as<const char*>());
		}	
 
		if(IrriObject->Selection[CurrentIrriSetup].SuspendMonID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendMon = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendTueID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendTue = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendWedID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendWed = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendThuID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendThu = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendFriID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendFri = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendSatID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendSat = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendSunID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendSun = String(kv.value().as<const char*>());
		}

		if(IrriObject->Selection[CurrentIrriSetup].SuspendALLID == String(kv.key().c_str()).toInt())
		{
			IrriObject->Selection[CurrentIrriSetup].SuspendALL = String(kv.value().as<const char*>());
		}

	}							
	WebMessUpdate(num, "Irrigation Updated!", "color: red", true);		
	ElecValves->SwitchValveConfig();
	IrriObject->WriteData(); 	
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////
////////handleResumeIrrig/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void handleResumeIrrig(AsyncWebServerRequest *request){
	//SaveGenReport("handleResumeIrrig " + String(ESP.getFreeHeap()));
	SetMess = FA_MESS_GRN;
	SpesialStr = "Irrigation has been Resumed";

	Lastnum = 0;
	LastnumUpdated = false;
	activateTimer(TIM_RESET_OPTION_MESS);

	SUSPEND_IRRI = false;
	RecordSuspendIrriSet("Manual");
    IrrigationPage(request);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////handleSuspendIrrig////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void handleSuspendIrrig(AsyncWebServerRequest *request){
	//SaveGenReport("handleSuspendIrrig " + String(ESP.getFreeHeap()));
	SetMess = FA_MESS_RED;
	SpesialStr = "Irrigation has been Suspended";

	Lastnum = 0;
	LastnumUpdated = false;
	activateTimer(TIM_RESET_OPTION_MESS);

	SUSPEND_IRRI = true;
	RecordSuspendIrriSet("Manual");
	IrrigationPage(request);
}

void CreateIrrigation() {
 	IrriObject = new Irrigation;	
	IrriObject->ReadData();		
}

// Class Irrigation
Irrigation::Irrigation() {}   // CONSTRUCTOR
Irrigation::~Irrigation() {}  // DESTRUCTOR

void Irrigation::Maintain() {
}

String Irrigation::GetDescription(int Line) {
	
	String Desc = "";
	if(this->Selection[Line].Status == IRRI_STATUS_INACTIVE)
	{
		return IRRI_STATUS_INACTIVE;		
	}
	else if (IrriObject->Selection[Line].SuspendALL == SUSPEND_YES ) 
	{
		return String(Desc + "SUSP. " + this->Selection[Line].SelectDescription );		
	}
	else
	{
		if(this->Selection[Line].OperMode == IRRI_MODE_MANUAL){Desc = "M ";}else{Desc = "S ";}
		return String(Desc + this->Selection[Line].PeriodToIrrigate + " " + this->Selection[Line].SelectDescription);
	}
}

String Irrigation::GetOption(int Line) {   

//	bool CheckNotScheduled = true;
	if(SheduleObject->IrrigateShedule[Line].ManualStop)
	{
		return IRRI_STATUS_START;			
	} 		
	else if(SheduleObject->IrrigateShedule[Line].ShedStat == SHED_STATUS_SCHEDULED)
	{
		return IRRI_STATUS_SHED;
	}
	else if(SheduleObject->IrrigateShedule[Line].ShedStat == SHED_STATUS_ACTIVE)
	{
		return IRRI_STATUS_STOP;
	}
	else
	{
		return IRRI_STATUS_START;		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////Irrigation::WriteData/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void Irrigation::WriteData()
{
	String StrFileName = StrIrrigation;	
    File ChannelDataFile = SPIFFS.open(StrFileName, "w");

    // Check if the file opened successfully
    if (!ChannelDataFile) {
        DEBUG_PRINTLN("Failed to open file for writing");
        return;
    }
	
    // Create a JSON document
    JsonDocument doc;

    // Create a JSON array to store the irrigation selections
    JsonArray irrigationArray = doc["irrigationSelections"].to<JsonArray>();

    // Iterate over each IrrigateSelection object and add its data to the JSON array
    for (int i = 0; i < IRRIGATE_SELECTIONS; i++) {
        JsonObject selectionObj = irrigationArray.add<JsonObject>();
        selectionObj["Status"] = Selection[i].Status;
        selectionObj["SelectDescription"] = Selection[i].SelectDescription;
        selectionObj["ConfigLine"] = Selection[i].ConfigLine;
        selectionObj["OperMode"] = Selection[i].OperMode;
        selectionObj["NextDateToIrrigate"] = Selection[i].NextDateToIrrigate;
        selectionObj["NextTimeToIrrigate"] = Selection[i].NextTimeToIrrigate;	
        selectionObj["PeriodToIrrigate"] = Selection[i].PeriodToIrrigate;			
        selectionObj["IrrigationFreq"] = Selection[i].IrrigationFreq;
        selectionObj["WaterSource"] = Selection[i].WaterSource;
        selectionObj["RespectRainfail"] = Selection[i].RespectRainfail;
        selectionObj["SuspendMon"] = Selection[i].SuspendMon;
        selectionObj["SuspendTue"] = Selection[i].SuspendTue;
        selectionObj["SuspendWed"] = Selection[i].SuspendWed;
        selectionObj["SuspendThu"] = Selection[i].SuspendThu;
        selectionObj["SuspendFri"] = Selection[i].SuspendFri;
        selectionObj["SuspendSat"] = Selection[i].SuspendSat;
        selectionObj["SuspendSun"] = Selection[i].SuspendSun;
        selectionObj["SuspendALL"] = Selection[i].SuspendALL;		
    }

    // Serialize the JSON document to a string
    String output;
    serializeJson(doc, output);

    // Write the serialized JSON to the file
    ChannelDataFile.print(output);

    // Close the file
    ChannelDataFile.close();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////Irrigation::ReadData//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

String IrrigateSelection::GetPeriodToIrrigateSeconds()
{
	return PeriodToIrrigate + ":00";
}

void Irrigation::ReadData()
{
	String StrFileName = StrIrrigation;	
 
	File ChannelDataFile = SPIFFS.open(StrFileName, "r");

    // Check if the file opened successfully
    if (!ChannelDataFile) {
        DEBUG_PRINTLN("Failed to open file for reading");
        return;
    }

    // Read the file contents into a string
    String fileContents = "";
    while (ChannelDataFile.available()) {
        char c = ChannelDataFile.read();
        fileContents += c;
    }

    // Close the file
    ChannelDataFile.close();

    // Parse the JSON string
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContents);

    // Check if parsing was successful
    if (error) {
        DEBUG_PRINTLN("Failed to parse JSON file");
        return;
    }

    // Retrieve the irrigation selections array from the JSON document
    JsonArray irrigationArray = doc["irrigationSelections"];

    // Iterate over each object in the array and update the variables of the IrrigateSelection objects
    for (int i = 0; i < irrigationArray.size(); i++) {
        JsonObject selectionObj = irrigationArray[i];
        Selection[i].Status = selectionObj["Status"].as<String>();		
        Selection[i].SelectDescription = selectionObj["SelectDescription"].as<String>();
        Selection[i].ConfigLine = selectionObj["ConfigLine"].as<String>();
        Selection[i].OperMode = selectionObj["OperMode"].as<String>();
        Selection[i].NextDateToIrrigate = selectionObj["NextDateToIrrigate"].as<String>();
        Selection[i].NextTimeToIrrigate = selectionObj["NextTimeToIrrigate"].as<String>();		
        Selection[i].PeriodToIrrigate = selectionObj["PeriodToIrrigate"].as<String>();				
        Selection[i].IrrigationFreq = selectionObj["IrrigationFreq"];
        Selection[i].WaterSource = selectionObj["WaterSource"].as<String>();
        Selection[i].RespectRainfail = selectionObj["RespectRainfail"].as<String>();
        Selection[i].SuspendMon = selectionObj["SuspendMon"].as<String>();
        Selection[i].SuspendTue = selectionObj["SuspendTue"].as<String>();
        Selection[i].SuspendWed = selectionObj["SuspendWed"].as<String>();
        Selection[i].SuspendThu = selectionObj["SuspendThu"].as<String>();
        Selection[i].SuspendFri = selectionObj["SuspendFri"].as<String>();
        Selection[i].SuspendSat = selectionObj["SuspendSat"].as<String>();
        Selection[i].SuspendSun = selectionObj["SuspendSun"].as<String>();	
        Selection[i].SuspendALL = selectionObj["SuspendALL"].as<String>();			
    }
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////
////////SHEDULING System//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////// 
 
void CreateSheduleObj() {
 	SheduleObject = new SheduleIrri;	
 	SheduleObject->Init();
}

// Class SheduleIrri
SheduleIrri::SheduleIrri() {}   // CONSTRUCTOR
SheduleIrri::~SheduleIrri() {}  // DESTRUCTOR
 
void SheduleIrri::Init() {
	for ( int i = 0 ; i < Lines; i++ ) {
		IrrigateShedule[i].ShedStat = SHED_STATUS_DEACTIVE;
		IrrigateShedule[i].StopDateTime = "";
		IrrigateShedule[i].StopMillis = 0;
		IrrigateShedule[i].ManualStop = false;
	}
}

//BDV
String GetScheduledDateAndTime(int Entry) { 
	String ScheduledDateTime = IrriObject->Selection[Entry].NextDateToIrrigate + " " +  IrriObject->Selection[Entry].NextTimeToIrrigate;
	return ScheduledDateTime;
}
 
void IrrigationInit() {
	String CurrentDateTime = GetCurrentDateAndTime();   // 02_Time.ino
	String ScheduledDateTime = "";
	for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
	{
		if(IrriObject->Selection[x].Status == IRRI_STATUS_ACTIVE && IrriObject->Selection[x].OperMode == IRRI_MODE_SHEDULE)
		{
			if(CurrentDateTime > GetScheduledDateAndTime(x))
			{	
				IrriObject->Selection[x].NextDateToIrrigate = GetCurrentDate(IrriObject->Selection[x].IrrigationFreq);
				IrriObject->WriteData(); 	 
			}
		}
	}
}

void UpdateToNextDate(int Entry) {

	String NewDate = IrriObject->Selection[Entry].NextDateToIrrigate;
	for ( int i = 0 ; i < IrriObject->Selection[Entry].IrrigationFreq; i++ ) 
	{
		NewDate = IncrementDateWithOneDay(NewDate); // 02_Time.ino
	}
	IrriObject->Selection[Entry].NextDateToIrrigate = NewDate;
	IrriObject->WriteData();
}

void IrrigationShedule() {  // From Main loop
	if (NormalOperation)  
	{	
		String CurrentDateTime = GetCurrentDateAndTime();   // 02_Time.ino	

		// When SUSPEND_IRRI = true
		if(SUSPEND_IRRI)
		{
			SuspendAllIrrigation();		
		}

		//Serial.print("1 Current  :   "); Serial.println(CurrentDateTime);	 
		// Monitor Lines to be Scheduled	
		String ScheduledDateTime = "";
		for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
		{
			if(IrriObject->Selection[x].Status == IRRI_STATUS_ACTIVE && 
			   IrriObject->Selection[x].OperMode == IRRI_MODE_SHEDULE && 
			   SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_DEACTIVE)
			{		
				ScheduledDateTime = GetScheduledDateAndTime(x);
				//Serial.print("2 Scheduled : "); Serial.print(x); Serial.print(" "); Serial.println(ScheduledDateTime);						
				if(CurrentDateTime >= ScheduledDateTime)
				{		
					//Serial.print("CurrentDateTime >= ScheduledDateTime "); Serial.println(x);
					if(SUSPEND_IRRI)
					{
						UpdateToNextDate(x);  // Irrigation has been suspended						
					}
					else
					{	
						if(!RAIN_OK_DO_IRRI && IrriObject->Selection[x].RespectRainfail == IRRI_RESP_RAIN_YES)
						{
							UpdateToNextDate(x); // It is raining AND Selection requires to respect it.
						}					
						else
						{
							String Suspend = SUSPEND_NO;
							switch (WeekDay)    
							{ 	
								case DAY_SUNDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendSun;
									break;	
								case DAY_MONDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendMon;
									break;									
								case DAY_TUEDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendTue;
									break;									
								case DAY_WEDDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendWed;
									break;									
								case DAY_THUDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendThu;
									break;	
								case DAY_FRIDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendFri;
									break;									
								case DAY_SATDAY: 	 
									Suspend = IrriObject->Selection[x].SuspendSat;
									break;									
							}	
							if(Suspend == SUSPEND_YES || IrriObject->Selection[x].SuspendALL == SUSPEND_YES )
							{
								UpdateToNextDate(x); // Day selecting require Suspention
							}
							else
							{
								SetupIrrigateShedule(x, SHED_TYPE_AUTO);
							}	
						}						
					}
				}			
			}
		}
		
		bool ReadyToShedule = true;    // TankFillChange 			
		// Monitor Lines to Stop
		for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
		{		
			if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_SCHEDULED && SheduleObject->IrrigateShedule[x].ManualStop)
			{	
				SheduleObject->IrrigateShedule[x].ManualStop = false;	
				SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_DEACTIVE;
			}
			if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_ACTIVE)
			{
				if(millis() >= SheduleObject->IrrigateShedule[x].StopMillis || SheduleObject->IrrigateShedule[x].ManualStop) {
					SheduleObject->IrrigateShedule[x].ManualStop = false;					
					SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_DEACTIVE;
					ActivateValves(IrriObject->Selection[x].ConfigLine.substring(7,8).toInt()-1, STATE_CLOSE_REQ);
					TankFillAllowed = true;
				}	
				else {
					ReadyToShedule = false;
				}
			}
		}
	
		// For Tank Shedulte Time 
		if(TankWaterLevel->TankFillingTime == TANK_FILL_OPER_SHEDULE) // "Scheduled Time"
		{			
			// In Fill Time
			if(GetCurrentTime() > TankWaterLevel->TankFillStartTime && GetCurrentTime() < TankWaterLevel->TankFillStopTime  ){  
				if (!InPort[I_FLOAT_HIGH].Input) ReadyToShedule = false;
			}
		}
		else  // Any Time
		{		
			if (!InPort[I_FLOAT_HIGH].Input) ReadyToShedule = false;
		}			

		// Monitor Lines for Start 
		if(ReadyToShedule && ValveLastState == VALVE_CLOSE) {   		
			for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
			{			
				if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_SCHEDULED)
				{					
					SheduleObject->IrrigateShedule[x].StopDateTime = AddTimeToCurrentDateTime(CurrentDateTime, IrriObject->Selection[x].PeriodToIrrigate);
					// Compute exact millis deadline from the HH:MM period string
					{
						String p = IrriObject->Selection[x].PeriodToIrrigate;
						unsigned long ms = (p.substring(0,2).toInt() * 3600UL + p.substring(3,5).toInt() * 60UL) * 1000UL;
						SheduleObject->IrrigateShedule[x].StopMillis = millis() + ms;
					}
					SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_ACTIVE;
					ActivateValves(IrriObject->Selection[x].ConfigLine.substring(7,8).toInt()-1, STATE_OPEN_REQ);
					x = Lines; // Terminate the for loop	
					TankFillAllowed = false;						
				}	
			}
		}				
	} 
}

void SuspendAllIrrigation(){
	
	for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ )
	{
		SheduleObject->IrrigateShedule[x].ManualStop = false;					
		if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_SCHEDULED)
		{	
			SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_DEACTIVE;
		}
		if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_ACTIVE)
		{
			SheduleObject->IrrigateShedule[x].ManualStop = false;					
			SheduleObject->IrrigateShedule[x].ShedStat = SHED_STATUS_DEACTIVE;
			ActivateValves(IrriObject->Selection[x].ConfigLine.substring(7,8).toInt()-1, STATE_CLOSE_REQ);
		}
	}
	TankFillAllowed = true;
}
 
void SetupIrrigateShedule(int Entry, String Type) { // Type = SHED_TYPE_AUTO or SHED_TYPE_MANUAL
	//    Line is Active                                           and Sheduler is Not Busy to Irrigate	
	if  ( IrriObject->Selection[Entry].Status == IRRI_STATUS_ACTIVE && 
	      SheduleObject->IrrigateShedule[Entry].ShedStat != SHED_STATUS_SCHEDULED ) 
	{
		// Set Line to be Scheduled to Irrigate 
		SheduleObject->IrrigateShedule[Entry].ShedStat = SHED_STATUS_SCHEDULED;
		if (Type == SHED_TYPE_AUTO)
		{	
			UpdateToNextDate(Entry);  
		}
	}			
} 