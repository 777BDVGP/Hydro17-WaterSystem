// =============================================================================
// 09_Suspend.cpp — Irrigation Suspension Day Management
// =============================================================================
// Implements the SuspendDays class (JSON persistence), the /managesuspend page
// handler, and MaintainSuspention() which evaluates the weekly suspension
// schedule each midnight and sets SUSPEND_IRRI accordingly.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handleManageSuspend  (GET /managesuspend)
// Serves the Suspension Day management page.
// ─────────────────────────────────────────────────────────────────────────────
void handleManageSuspend(AsyncWebServerRequest *request){
	//SaveGenReport("handleManageSuspend " + String(ESP.getFreeHeap()));
	if (!SuspendObject) CreateSuspendMaps();
	CompilePage_Suspend(request);
}

void CompilePage_Suspend(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	IrrigatePageSel(PS_IRRIGATION_SUSPEND, response);
	response->print(webScriptTag);
	WT_SETUP_Suspend(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

void MapSuspendPage(){	
//  Create as Synchronisation array of Objects, SuspendList, to setup to connection to the Webpage variables    
	SuspendList = new PageVar[SUSPEND_DAYS*2];	
	for (int i=0; i < SUSPEND_DAYS; i++) 
	{ 
		SuspendList[i].Var = new PageElement(&DaysSuspend->SuspendingDay[i].Suspend, DaysSuspend->SuspendingDay[i].Day, "", &SetStyle, i+ID_SUSPEND);  
		SuspendList[i+SUSPEND_DAYS].Var = new PageElement(&DaysSuspend->SuspendingDay[i].Once, "", "", &SetStyle, i+SUSPEND_DAYS+ID_SUSPEND); 
	}
}

void WT_SETUP_Suspend(AsyncResponseStream* response){
	resetPageBuf();
	WT_StartPage();
	WT_StartTable("Weekday", "Suspend", "Once");
	for (int i = 0; i < SUSPEND_DAYS; i++){
		WT_StartLine();
		WT_AutoColumn(WT_LABEL, WT_FIRST, WT_LAST, &SuspendList[i], "L");
		WT_AutoColumn(WT_INPUT, WT_FIRST, WT_MORE, &SuspendList[i]);
		WT_AutoColumn(WT_INPUT, WT_NEXT,  WT_LAST, &SuspendList[i + SUSPEND_DAYS]);
		WT_EndLine();
	}
	WT_EndPageClass("susp");
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

 void MaintainSuspendPageValues(){	
//  Any Changes of display variables on the WEB Page needs to be updated immediately
	for (int i=0; i < SUSPEND_DAYS*2; i++) 
	{ 
		if(SuspendList[i].Var->GetName() != SuspendList[i].Var->GetNameLast())
		{
			webParamUpdate(SuspendList[i].Var->GetName(),SuspendList[i].Var->GetID(""),SuspendList[i].Var->GetVarStyle()); SuspendList[i].Var->UpdateLast();						
		}
	}	
}

void UpdateSuspendPageValues(uint32_t num, JsonObject *root){
// When the UPDATE button is pressed, all variable on the WEB Page, needs to be updated in the server.	 
	for (JsonPair kv : *root){	
		//	Serial.print("Num: ");Serial.print(num); 		
		//	Serial.print(" Key: ");Serial.print(kv.key().c_str()); 
		//	Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 	
		for (int i=0; i < SUSPEND_DAYS*2; i++) 
		{ 
			if((SuspendList[i].Var->GetID("") == kv.key().c_str()))				
			{	
				SuspendList[i].Var->SetName(String(kv.value().as<const char*>()));	 
			}	
		}	
	}
	WebMessUpdate(num, "Suspend Updated!", "color: red", true);	
	DaysSuspend->WriteData();
 }

// Class SuspendDays

// Create SuspendDays Object
void CreateSuspendDays() {
	DaysSuspend = new SuspendDays;	
	DaysSuspend->Init();
}	
// Create Maping objects for Valve & Config Web Displays  
void CreateSuspendMaps() {
	SuspendObject = true;
	MapSuspendPage();	
}

SuspendDays::SuspendDays() {}   // CONSTRUCTOR
SuspendDays::~SuspendDays() {}  // DESTRUCTOR

void SuspendDays::Init() {
 
	SuspendingDay[SUSPEND_1].Day = "Sunday";  
	SuspendingDay[SUSPEND_2].Day = "Monday";	
	SuspendingDay[SUSPEND_3].Day = "Tuesday";
	SuspendingDay[SUSPEND_4].Day = "Wednesday";	
	SuspendingDay[SUSPEND_5].Day = "Thursday";	
	SuspendingDay[SUSPEND_6].Day = "Friday";	
	SuspendingDay[SUSPEND_7].Day = "Saturday";	
 
	SuspendingDay[SUSPEND_1].Weekday = 1;
	SuspendingDay[SUSPEND_2].Weekday = 2;
	SuspendingDay[SUSPEND_3].Weekday = 3;
	SuspendingDay[SUSPEND_4].Weekday = 4;
	SuspendingDay[SUSPEND_5].Weekday = 5;
	SuspendingDay[SUSPEND_6].Weekday = 6;
	SuspendingDay[SUSPEND_7].Weekday = 7; 
	
	DaysSuspend->ReadData();	
}

void SuspendDays::WriteData() {
	File ChannelDataFile = SPIFFS.open(StrSuspendDays, "w");
	JsonDocument doc; 

	String docDesc;
	for (int i=0; i < SUSPEND_DAYS; i++){ 
		docDesc = F("Suspend_"); docDesc += i;	
		doc[docDesc] = SuspendingDay[i].Suspend;	
		docDesc = F("Once_"); docDesc += i;	
		doc[docDesc] = SuspendingDay[i].Once;			
	}

	if (ChannelDataFile) 
	{   
		serializeJsonPretty(doc, ChannelDataFile);  
		ChannelDataFile.close(); 
	}	
}

void SuspendDays::ReadData() {
	File ChannelDataFile = SPIFFS.open(StrSuspendDays, "r");	
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
				String docDesc;
				for (int i=0; i < SUSPEND_DAYS; i++){ 
					docDesc = F("Suspend_"); docDesc += i;	
					if ( doc[docDesc].as<String>() != "null" ) SuspendingDay[i].Suspend = doc[docDesc];
					docDesc = F("Once_"); docDesc += i;					
					if ( doc[docDesc].as<String>() != "null" ) SuspendingDay[i].Once = doc[docDesc];							
				}
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrSuspendDays);
			}
		}
		else
		{
			DEBUG_PRINT(StrSuspendDays);  DEBUG_PRINTLN(" File size is > 1024 ");
		}
		ChannelDataFile.close();  	
	}
} 

// ─────────────────────────────────────────────────────────────────────────────
// MaintainSuspention  (called from CompileDateTime — on reboot and at midnight)
// Rule a (always):   Read today's Suspend flag → set SUSPEND_IRRI.
//                    Because Suspend stays true in JSON until explicitly cleared,
//                    a mid-day reboot still picks up the correct suspension state.
// Rule b (midnight): Clear YESTERDAY's Suspend+Once if Once was set.
//                    WeekDay has already advanced to the new day, so yesterday's
//                    index is (WeekDay-2+7)%7.  This guarantees the flag remains
//                    in JSON throughout the suspended day and is only removed once
//                    that day has fully ended.
//                    Never runs on reboot (isMidnight == false).
// ─────────────────────────────────────────────────────────────────────────────
void MaintainSuspention(bool isMidnight) {

	// ── Rule a — applies to both reboot and midnight ──────────────────────────
	SUSPEND_IRRI = DaysSuspend->SuspendingDay[WeekDay - 1].Suspend;
	if (SUSPEND_IRRI) RecordSuspendIrriSet("Auto");

	// ── Rule b — midnight only: clear the day that just ended ────────────────
	// WeekDay is already the new day, so yesterday = (WeekDay-2+7) % 7 (0-based).
	if (isMidnight) {
		int yesterday = (WeekDay - 2 + 7) % 7;
		if (DaysSuspend->SuspendingDay[yesterday].Once) {
			DaysSuspend->SuspendingDay[yesterday].Suspend = false;
			DaysSuspend->SuspendingDay[yesterday].Once    = false;
			DaysSuspend->WriteData();
		}
	}
}

void RecordSuspendIrriSet(String Msg){ 
	String msg = "";
	if(Msg == "Auto")
	{
		msg = "Scheduled Irrigation Suspended ON: ";  msg += GetCurrentDateAndTime();
	}
	else
	{
		if(SUSPEND_IRRI) {
			msg = "Manual Irrigation Suspended  ON: ";  msg += GetCurrentDateAndTime();
		} else {
			msg = "Manual Irrigation Suspended OFF: ";  msg += GetCurrentDateAndTime();
		}			
	}
	SaveIrrigReport (msg);		
}