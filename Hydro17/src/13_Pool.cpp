// =============================================================================
// 13_Pool.cpp — Pool Filter Pump Scheduler & Pool Light Control
// =============================================================================
// Implements the PoolShedule class (JSON persistence), all pool page handlers
// (/pool, /switchonPool, /switchoffPool, /switchonlight, /switchofflight),
// and MaintainPoolFilterPump() — the state machine that starts and stops the
// pool filter pump according to the weekly schedule or manual override.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handlePoolOn  (GET /switchonPool)
// Manually activates the pool filter pump and redirects to the Pool page.
// ─────────────────────────────────────────────────────────────────────────────
void handlePoolOn(AsyncWebServerRequest *request){
	POOL_FILTER_ON = true;
	PoolOverride   = POOL_OVR_FORCED_ON;
	handlePool(request);
}

void handlePoolOff(AsyncWebServerRequest *request){
	POOL_FILTER_ON = false;
	PoolOverride   = POOL_OVR_FORCED_OFF;
	handlePool(request);
}

void handleLightOn(AsyncWebServerRequest *request){
	//SaveGenReport("handleLightOn " + String(ESP.getFreeHeap()));
    POOL_LIGHT_ON = true;
	LightManual = true;
	handlePoolLight(request);
}

void handleLigthOff(AsyncWebServerRequest *request){
	//SaveGenReport("handleLigthOff " + String(ESP.getFreeHeap()));
    POOL_LIGHT_ON = false;
	LightManual = true;
	handlePoolLight(request);
}

void handleLightManOff(AsyncWebServerRequest *request){
	//SaveGenReport("handleLightManOff " + String(ESP.getFreeHeap()));
	LightManual = false;
	handlePoolLight(request);
}

void handlePool(AsyncWebServerRequest *request){
	//SaveGenReport("handlePool " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_PoolMenu(response);
	response->print(webScriptTag);
	WT_PoolPage(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_UL_PoolMenu(AsyncResponseStream* r) {
	r->print(F("<div class='nav'>"));
	r->print(F("<a href='/'>HOME</a>"));
	r->print(F("<a class='act'>POOL FILTER</a>"));
	r->print(POOL_FILTER_ON ? "<a class='warn' href='/switchoffPool'>SWITCH OFF POOL FILTER</a>"
	                        : "<a class='warn' href='/switchonPool'>SWITCH ON POOL FILTER</a>");
	r->print(F("</div>"));
}

void WT_PoolPage(AsyncResponseStream* response){
// 	Create List of Setting Page Parameters
	resetPageBuf();

 	WT_StartPage();
	WT_Header("WEEK DAY"); WT_Header("FROM"); WT_Header("TO"); WT_Header("ACTIVE");
	int ID = ID_POOL;
	for (int j=0; j < WEEK_DAYS ; j++){
		char idS[8]; char idE[8]; char idA[8];
		snprintf(idS, sizeof(idS), "%d", ID); ID++;
		snprintf(idE, sizeof(idE), "%d", ID); ID++;
		snprintf(idA, sizeof(idA), "%d", ID); ID++;
		const char* sel = PoolFilter->Scheduleday[j].Select ? "true" : "false";
		WT_StartLine();
		WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text",     DayDesc[j].c_str(),                          "",    "", nullptr, 0);
		WT_Column(WT_INPUT, WT_FIRST, WT_MORE, "time",     PoolFilter->Scheduleday[j].Start.c_str(),     idS,   "", nullptr, 0);
		WT_Column(WT_INPUT, WT_NEXT,  WT_MORE, "time",     PoolFilter->Scheduleday[j].Stop.c_str(),      idE,   "", nullptr, 0);
		WT_Column(WT_INPUT, WT_NEXT,  WT_LAST, "checkbox", sel,                                          idA,   "", nullptr, 0);
		WT_EndLine();
	}
 	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

String SetBool(bool Val){
	if(Val == true){ return "true"; } else { return "false"; }
}

void UpdatePoolPageValues(uint32_t num, JsonObject *root){
	int ID = ID_POOL;
	for (JsonPair kv : *root){	
 		// Serial.print("Num: "); Serial.print(num); 		
 		// Serial.print(" Key: "); Serial.print(kv.key().c_str()); 
 		// Serial.print(" Value: "); Serial.println(kv.value().as<const char*>()); 
		int keyValue = atoi(kv.key().c_str());	
		String RetValue = String(kv.value().as<const char*>());	
		ID = ID_POOL;
		for (int j=0; j < WEEK_DAYS; j++){	
			if( keyValue == ID ) {
				PoolFilter->Scheduleday[j].Start = RetValue;			
			}
			ID++;		
			if( keyValue == ID ) {
				PoolFilter->Scheduleday[j].Stop = RetValue;			 			
			}
			ID++;
			if( keyValue == ID ) {					
				if( RetValue == "true") {PoolFilter->Scheduleday[j].Select = true;} else {PoolFilter->Scheduleday[j].Select = false;}	 
			}		
			ID++;
		}	
	}
	WebMessUpdate(num, "Pool Parameters Updated!", "color: red", true);		
	PoolFilter->WriteData();

 }
 ///////////////////////////////////////////////////////////////////////////////////////////
 
 // Class PoolShedule

// Create PoolShedule Object
void CreatePoolShedule() {
	PoolFilter = new PoolShedule;	
	PoolFilter->ReadData();
}	

PoolShedule::PoolShedule() {}   // CONSTRUCTOR
PoolShedule::~PoolShedule() {}  // DESTRUCTOR

void PoolShedule::WriteData() {
	File ChannelDataFile = SPIFFS.open(StrPoolFilFile, "w");
	JsonDocument doc; 

	String docDesc;
	for (int i=0; i < WEEK_DAYS; i++){ 
		docDesc = F("Start_"); docDesc += i;	
		doc[docDesc] = Scheduleday[i].Start;	
		docDesc = F("Stop_"); docDesc += i;	
		doc[docDesc] = Scheduleday[i].Stop;			
		docDesc = F("Select_"); docDesc += i;	
		doc[docDesc] = Scheduleday[i].Select;					
	}

	if (ChannelDataFile) 
	{   
		serializeJsonPretty(doc, ChannelDataFile);  
		ChannelDataFile.close(); 
	}	
}

void PoolShedule::ReadData() {
	File ChannelDataFile = SPIFFS.open(StrPoolFilFile, "r");	
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
				for (int i=0; i < WEEK_DAYS; i++){ 
					docDesc = F("Start_"); docDesc += i;	
				    if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Start = doc[docDesc].as<String>();   
					docDesc = F("Stop_"); docDesc += i;	
				 	if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Stop = doc[docDesc].as<String>();
					docDesc = F("Select_"); docDesc += i;	
				 	if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Select = doc[docDesc];									
				}
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrPoolFilFile);
			}
		}
		else
		{
			DEBUG_PRINT(StrPoolFilFile);  DEBUG_PRINTLN(" File size is > 1024 ");
		}
		ChannelDataFile.close();  	
	}
} 

void MaintainPoolFilterPump(){

	int DayOfWeek = WeekDay-1;  //  0 = Sunday … 6 = Saturday

	// ── 19:00 cutover: FORCED_ON → FORCED_OFF ────────────────────────────────
	if (PoolOverride == POOL_OVR_FORCED_ON && GetCurrentTime() >= "19:00") {
		PoolOverride = POOL_OVR_FORCED_OFF;
	}

	// ── Determine pump state from override or weekly schedule ─────────────────
	if (PoolOverride == POOL_OVR_FORCED_ON) {
		POOL_FILTER_ON = true;
	} else if (PoolOverride == POOL_OVR_FORCED_OFF) {
		POOL_FILTER_ON = false;
	} else {
		// POOL_OVR_NONE — weekly schedule is in control
		if (PoolFilter->Scheduleday[DayOfWeek].Select) {
			POOL_FILTER_ON = (GetCurrentTime() >= PoolFilter->Scheduleday[DayOfWeek].Start &&
			                  GetCurrentTime() <  PoolFilter->Scheduleday[DayOfWeek].Stop);
		}
	}

	// ── Physical button ───────────────────────────────────────────────────────
	if (InPort[I_POOL_FILTER].Input)
	{
		if (POOL_FILTER_BUT)
		{
			if (POOL_FILTER_ON) {
				POOL_FILTER_ON = false;
				PoolOverride   = POOL_OVR_FORCED_OFF;   // off until tomorrow
			} else {
				POOL_FILTER_ON = true;
				PoolOverride   = POOL_OVR_FORCED_ON;    // on until 19:00
			}
		}
		POOL_FILTER_BUT = false;
	}
	else
	{
		POOL_FILTER_BUT = true;
	}
	
 	if( InPort[I_POOL_LIGHT].Input)  // Pool Light On
	{
		if( POOL_LIGHT_BUT)
		{
			POOL_LIGHT_ON = !POOL_LIGHT_ON;
			LightManual = true;
		}
		 POOL_LIGHT_BUT = false;
	}
	else
	{
		 POOL_LIGHT_BUT = true;
	}

	// Pool light schedule (only when not in manual override)
	// State is determined entirely by the time window — no unconditional OFF
	// so that the light stays on when RESET AUTO SCHEDULE is pressed.
	if(PoolLight->Scheduleday[DayOfWeek].Select && !LightManual)
	{
		if( GetCurrentTime() >= PoolLight->Scheduleday[DayOfWeek].Start &&
		    GetCurrentTime() <  PoolLight->Scheduleday[DayOfWeek].Stop )
		{
			POOL_LIGHT_ON = true;
		}
		else
		{
			POOL_LIGHT_ON = false;
		}
	}

	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_POOL_PUMP_RELAY].OutputPin)].Port, POOL_FILTER_ON);  
	digitalWrite(OutPort[(ButtonObject->Selection[BUTTON_POOL_LIGHT_RELAY].OutputPin)].Port, POOL_LIGHT_ON); 	
}


// =============================================================================
// Pool Light Page
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handlePoolLight  (GET /poollight)
// Renders the Pool Light schedule page.
// ─────────────────────────────────────────────────────────────────────────────
void handlePoolLight(AsyncWebServerRequest *request){
	//SaveGenReport("handlePoolLight " + String(ESP.getFreeHeap()));
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	append_UL_PoolLightMenu(response);
	response->print(webScriptTag);
	WT_PoolLightPage(response);
	response->print(webUpateButton);
	response->print(webHTMLEnd);
	request->send(response);
}

// ─────────────────────────────────────────────────────────────────────────────
// append_UL_PoolLightMenu
// Builds the navigation tab strip for the Pool Light page.
// ─────────────────────────────────────────────────────────────────────────────
void append_UL_PoolLightMenu(AsyncResponseStream* r) {
	r->print(F("<div class='nav'>"));
	r->print(F("<a href='/'>HOME</a>"));
	r->print(F("<a class='act'>POOL LIGHT</a>"));
	r->print(POOL_LIGHT_ON ? "<a class='warn' href='/switchofflight'>SWITCH OFF POOL LIGHT</a>"
	                       : "<a class='warn' href='/switchonlight'>SWITCH ON POOL LIGHT</a>");
	if (LightManual) r->print(F("<a class='warn' href='/switchoffManLight'>RESET AUTO SCHEDULE</a>"));
	r->print(F("</div>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// WT_PoolLightPage
// Builds the weekly schedule table for the pool light.
// ─────────────────────────────────────────────────────────────────────────────
void WT_PoolLightPage(AsyncResponseStream* response){
	resetPageBuf();

	WT_StartPage();
	WT_Header("WEEK DAY"); WT_Header("FROM"); WT_Header("TO"); WT_Header("ACTIVE");
	int ID = ID_POOL_LIGHT;
	for (int j=0; j < WEEK_DAYS ; j++){
		char idS[8]; char idE[8]; char idA[8];
		snprintf(idS, sizeof(idS), "%d", ID); ID++;
		snprintf(idE, sizeof(idE), "%d", ID); ID++;
		snprintf(idA, sizeof(idA), "%d", ID); ID++;
		const char* sel = PoolLight->Scheduleday[j].Select ? "true" : "false";
		WT_StartLine();
		WT_Column(WT_LABEL, WT_FIRST, WT_LAST, "text",     DayDesc[j].c_str(),                         "",    "", nullptr, 0);
		WT_Column(WT_INPUT, WT_FIRST, WT_MORE, "time",     PoolLight->Scheduleday[j].Start.c_str(),     idS,   "", nullptr, 0);
		WT_Column(WT_INPUT, WT_NEXT,  WT_MORE, "time",     PoolLight->Scheduleday[j].Stop.c_str(),      idE,   "", nullptr, 0);
		WT_Column(WT_INPUT, WT_NEXT,  WT_LAST, "checkbox", sel,                                         idA,   "", nullptr, 0);
		WT_EndLine();
	}
	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdatePoolLightPageValues
// Receives WebSocket JSON updates for the pool light schedule and persists them.
// ─────────────────────────────────────────────────────────────────────────────
void UpdatePoolLightPageValues(uint32_t num, JsonObject *root){
	int ID = ID_POOL_LIGHT;
	for (JsonPair kv : *root){
		int keyValue = atoi(kv.key().c_str());
		String RetValue = String(kv.value().as<const char*>());
		ID = ID_POOL_LIGHT;
		for (int j=0; j < WEEK_DAYS; j++){
			if( keyValue == ID ) {
				PoolLight->Scheduleday[j].Start = RetValue;
			}
			ID++;
			if( keyValue == ID ) {
				PoolLight->Scheduleday[j].Stop = RetValue;
			}
			ID++;
			if( keyValue == ID ) {
				if( RetValue == "true") {PoolLight->Scheduleday[j].Select = true;} else {PoolLight->Scheduleday[j].Select = false;}
			}
			ID++;
		}
	}
	WebMessUpdate(num, "Pool Light Parameters Updated!", "color: red", true);
	PoolLight->WriteData();
}

// =============================================================================
// PoolLightShedule class implementation
// =============================================================================

void CreatePoolLightShedule() {
	PoolLight = new PoolLightShedule;
	PoolLight->ReadData();
}

PoolLightShedule::PoolLightShedule() {}
PoolLightShedule::~PoolLightShedule() {}

void PoolLightShedule::WriteData() {
	File ChannelDataFile = SPIFFS.open(StrPoolLightFile, "w");
	JsonDocument doc;

	String docDesc;
	for (int i=0; i < WEEK_DAYS; i++){
		docDesc = F("Start_"); docDesc += i;
		doc[docDesc] = Scheduleday[i].Start;
		docDesc = F("Stop_"); docDesc += i;
		doc[docDesc] = Scheduleday[i].Stop;
		docDesc = F("Select_"); docDesc += i;
		doc[docDesc] = Scheduleday[i].Select;
	}

	if (ChannelDataFile)
	{
		serializeJsonPretty(doc, ChannelDataFile);
		ChannelDataFile.close();
	}
}

void PoolLightShedule::ReadData() {
	File ChannelDataFile = SPIFFS.open(StrPoolLightFile, "r");
	if (ChannelDataFile)
	{
		size_t size = ChannelDataFile.size();
		if (size <= 1024)
		{
			std::unique_ptr<char[]> buf(new char[size]);
			ChannelDataFile.readBytes(buf.get(), size);
			JsonDocument doc;
			auto error = deserializeJson(doc, buf.get());
			if (!error)
			{
				String docDesc;
				for (int i=0; i < WEEK_DAYS; i++){
					docDesc = F("Start_"); docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Start = doc[docDesc].as<String>();
					docDesc = F("Stop_"); docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Stop = doc[docDesc].as<String>();
					docDesc = F("Select_"); docDesc += i;
					if ( doc[docDesc].as<String>() != "null" ) Scheduleday[i].Select = doc[docDesc];
				}
			}
			else
			{
				DEBUG_PRINT("Error deserialize "); DEBUG_PRINTLN(StrPoolLightFile);
			}
		}
		else
		{
			DEBUG_PRINT(StrPoolLightFile); DEBUG_PRINTLN(" File size is > 1024 ");
		}
		ChannelDataFile.close();
	}
}
