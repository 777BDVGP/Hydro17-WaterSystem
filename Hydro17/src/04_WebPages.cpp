
// =============================================================================
// 04_WebPages.cpp — MaintainWebService, Web Server Setup & WebSocket Events
// =============================================================================
// Implements MaintainWebService (web/OTA upkeep, driven from loop()),
// the AsyncWebServer and WebSocket initialisation, JSON payload parsing, and the
// web table builder helper functions used by all page modules.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// MaintainWebService  (called every pass of loop(), self-timed internally)
// Polls ArduinoOTA at 5 Hz (loop() takes over at full rate once a transfer
// starts), cleans up WebSocket clients and pushes changed values at 20 Hz,
// refreshes the date/time display, and updates home-page status at 2 Hz.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainWebService() {
	static uint32_t lastOTACheck = 0;  // Throttle ArduinoOTA.handle() to 5 Hz
	static uint32_t lastWebPage  = 0;  // Throttle MaintainWebPage()   to 20 Hz
	static uint32_t lastHomePage = 0;  // Throttle MaintainHomePage()  to 2 Hz
	uint32_t now = millis();

	if (now - lastOTACheck >= 200) {
		lastOTACheck = now;
		ArduinoOTA.handle();
	}

	if (OTAInProgress) return;

	// Throttle to 20 Hz — running at 100 Hz burns CPU on String comparisons
	// every cycle and floods the WebSocket with micro-updates that the browser
	// can't consume fast enough, fragmenting the DRAM heap.
	if (now - lastWebPage >= 50) {
		lastWebPage = now;
		MaintainWebPage();    // Cleanup clients, push changed values (04_WebPages.cpp)
	}
	UpdateWebPageTime();      // Refresh date/time display             (02_Time.cpp)

	if (now - lastHomePage >= 500) {
		lastHomePage = now;
		MaintainHomePage();   // Refresh home-page status strings      (08_Home.cpp)
	}
}

void SetupWebPage(){
//	Add Routes for this System
	AddRoutes();

//  Write the WebSocket JavaScript to SPIFFS so pages can load it as a cached
//  static file.  webScriptTag embeds the build date as a query parameter
//  (?v=__DATE__) so the browser fetches fresh JS after every firmware update,
//  while caching it for 24 hours within the same build.
	{
		File f = SPIFFS.open("/wss.js", "w");
		if (f) { f.print(webSocketScript); f.close(); }
		else    { DEBUG_PRINTLN("SetupWebPage: failed to write /wss.js"); }
	}
	server.serveStatic("/wss.js", SPIFFS, "/wss.js").setCacheControl("max-age=86400");

//  Write the CSS stylesheet to SPIFFS so pages can load it as a cached static
//  file.  webHeadPage embeds the build date as a ?v=__DATE__ cache-buster so
//  the browser fetches fresh CSS after every OTA update.
	{
		File f = SPIFFS.open("/style.css", "w");
		if (f) { f.print(webStyleSheet); f.close(); }
		else    { DEBUG_PRINTLN("SetupWebPage: failed to write /style.css"); }
	}
	server.serveStatic("/style.css", SPIFFS, "/style.css").setCacheControl("max-age=86400");

//  Attach WebSocket event handler and register with server
	ws.onEvent(webSocketEvent);
	server.addHandler(&ws);

//  Start Server
	server.begin();
}

void MaintainWebPage(){

	ws.cleanupClients(2);

//  Maintain Values on Web Pages
	MaintainWebPageValues();	// 05_ThisSystem.ino

//	Reset to "Selection Option"
	UpdateSelectMessDelay();
}

void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){

	switch (type){
		case WS_EVT_DISCONNECT:
		DEBUG_PRINTLN(F("Web Page Disconnected"));
		break;

		case WS_EVT_CONNECT: {
			// Push current system name and date/time to newly connected browser
			String ActTime = DateTime;
			webParamUpdate(Params->SysName, "H01", "");
			webParamUpdate(ActTime, "H02", "");
		}
		break;

		case WS_EVT_DATA: {
			AwsFrameInfo *info = (AwsFrameInfo*)arg;
			// Only process complete, single-frame text messages
			if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
				data[len] = 0;   // Null-terminate for safe JSON parsing
				UnpackJasonPayload(client->id(), data, len);
			}
		}
		break;

		case WS_EVT_ERROR:
		case WS_EVT_PONG:
		break;
	}
}

void webParamUpdate(const String& Name, const String& ID, const String& ATT, const char* ATTName){
	if(ws.count() == 0) return;
	JsonDocument doc;
	doc[ID] = Name;
	if(ATT.length()){
		doc["Attribute"] = ATT;
		if(strcmp(ATTName,"style") != 0) doc["AttributeName"] = ATTName;
	}
	char buf[256];
	serializeJson(doc, buf, sizeof(buf));
	ws.textAll(buf);
}

void UnpackJasonPayload(uint32_t num, uint8_t * payload, size_t length){  
	// Create a JSON object to store the unpacked JSON string
	JsonDocument doc;

	// Parse the payload into the JSON object
	DeserializationError error = deserializeJson(doc, payload);
	if (error){
		SaveGenReport F("Error parsing JSON payload");	
		return; 
	}
	
	// Iterate over the keys in the JSON object
	JsonObject root = doc.as<JsonObject>();

	// Find the first numeric key (>0) to identify which page sent this payload.
	// Non-numeric keys (e.g. "dpool", "Update", "REBOOT") from auxiliary inputs
	// on the same page are skipped so they don't misroute the dispatch.
	int WEBPAGE = 0;
	for (JsonPair kv : root) {
		int id = String(kv.key().c_str()).toInt();
		if (id > 0) { WEBPAGE = id; break; }
	}
	// TANK_RAW_VALUE (index 0, ID 350) is display-only so generates no <input>.
	// The first real input is TANK_SENSOR_100 (index 1, ID 351). Normalise any
	// tank-range ID back to ID_TANK so the switch routes correctly.
	if (WEBPAGE > ID_TANK && WEBPAGE < ID_POOL) WEBPAGE = ID_TANK;

	//Serial.print("UNPACK WEBPAGE "); Serial.println(WEBPAGE);
	
	//	Updating of the WEBSOCKETS pages  	
	switch (WEBPAGE)    
	{ 	
		case ID_HOME: 
			// No Updates
			break;
		case ID_SETTINGS: 	
			//Serial.println("ID_SETTINGS");
			UpdateSettingsPageValues(num, &root);	 
			break;	
		case ID_SUSPEND: 	
			//Serial.println("ID_SUSPEND");
			if(SuspendObject) UpdateSuspendPageValues(num, &root);
			break;	
		case ID_VALVES:
		    //Serial.println("ID_VALVES");
			UpdateWaterValvesPageValues(num, &root);
			break;
		case ID_VALCONF:
		    //Serial.println("ID_VALCONF");
			UpdateConfigPageValues(num, &root);			
			break;	
		case ID_IRRISETUP:
		    //Serial.println("ID_IRRISETUP");
			UpdateIrrigSetupPageValues(num, &root);			
			break;		
		case ID_TANK:		
		    //Serial.println("ID_TANK");	
			UpdateTankPageValues(num, &root);  
			break;				
		case ID_POOL:
		    //Serial.println("ID_POOL");
			UpdatePoolPageValues(num, &root);
			break;
		case ID_POOL_LIGHT:
		    //Serial.println("ID_POOL_LIGHT");
			UpdatePoolLightPageValues(num, &root);
			break;
		case ID_NTFY:
			//Serial.println("ID_NTFY");
			if(NtfyPageObject) UpdateNtfyPageValues(num, &root);
			break;
		case 0:	
			for (JsonPair kv : root){		
				//Serial.print("Num: ");Serial.print(num); 		
				//Serial.print(" Key: ");Serial.print(kv.key().c_str()); 
				//Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 
				String Key = kv.key().c_str();
				if(kv.key() == "REBOOT" && kv.value() == "YES")
				{
					DEBUG_PRINTLN("Manual Rebooted");
					WebMessUpdate(num, "Rebooted, Communications lost!", "color: red", true);
					SaveGenReport("REBOOT - Manual reboot via WebSocket");
					delay(500);
					ESP.restart();
				}
			}	
			break;				
	}
}
 
void WebMessUpdate(uint32_t num, const String& Mess, const String& Att, bool Reset){
	webParamUpdate(Mess, "H03", Att);
	if(Reset)
	{
		Lastnum = num;
		LastnumUpdated = false;
		activateTimer(TIM_RESET_OPTION_MESS);
	}
}

void UpdateSelectMessDelay(){ // Called cyclically to RESET 
	if(!LastnumUpdated)
	{
		if(TimerExpired(TIM_RESET_OPTION_MESS)){
			WebMessUpdate(Lastnum, "Selection Option", "color:#3366ff", false);	
			LastnumUpdated = true;			
		}
	}
}

void append_FileOption(int ReqMess, AsyncResponseStream* r)
{
	const char* Message = "Selection Option";
	const char* LColor  = "color:#3366ff";
	bool useSpesial = false;
	bool spesialRed = false;
	int ActMess = ReqMess;
	if (SetMess != 0) ActMess = SetMess;
	switch (ActMess) {
		case FA_MESS_01: Message = "Files on SPIFFS";                    LColor = "color:#227c91"; break;
		case FA_MESS_02: Message = "Click on File to Download";          LColor = "color:#3366ff"; break;
		case FA_MESS_03: Message = "Upload a File";                      LColor = "color:#3366ff"; break;
		case FA_MESS_04: Message = "Click on File to Delete";            LColor = "color:#3366ff"; break;
		case FA_MESS_06: Message = "Selection Option";                   LColor = "color:#3366ff"; break;
		case FA_MESS_07: Message = "Files on SPIFFS";                    LColor = "color:#3366ff"; break;
		case FA_MESS_08: Message = "Select Report";                      LColor = "color:#3366ff"; break;
		case FA_MESS_09: Message = "File Not Found";                     LColor = "color:#ff0000"; break;
		case FA_MESS_10: Message = "File GeneralLog.txt was deleted";    LColor = "color:#ff0000"; break;
		case FA_MESS_11: Message = "File IrrigationLog.txt was deleted"; LColor = "color:#ff0000"; break;
		case FA_MESS_RED: useSpesial = true; spesialRed = true;  break;
		case FA_MESS_GRN: useSpesial = true; spesialRed = false; break;
	}
	r->print(F("<h2 id=\"H01\">")); r->print(Params->SysName); r->print(F("</h2>"));
	r->print(F("<h4 id=\"H02\">")); r->print(DateTime);         r->print(F("</h4>"));
	r->print(F("<h4 id=\"H03\" style="));
	if (useSpesial) {
		r->print(spesialRed ? "color:#ff0000" : "color:#33cc33");
		r->print(F(">")); r->print(SpesialStr);
	} else {
		r->print(LColor); r->print(F(">")); r->print(Message);
	}
	r->print(F("</h4>"));
	SetMess = 0;
}

////// START - Page generation for a table //////
void WT_StartTable(const char* Hdr1, const char* Hdr2, const char* Hdr3){
	WT_Header(Hdr1);
	if(Hdr2[0]) { WT_Header(Hdr2); }
	if(Hdr3[0]) { WT_Header(Hdr3); }
}



// ─── WT_* primitive emitters ──────────────────────────────────────────────────
void WT_StartPage()          { appendToPageBuf("<script>const parameters = ["); }
void WT_EndPage()            { appendToPageBuf("]; createLabelInputTable(parameters); </script>"); }
void WT_EndPageClass(const char* cls) {
	appendToPageBuf("]; createLabelInputTable(parameters,\""); appendToPageBuf(cls); appendToPageBuf("\"); </script>");
}
void WT_Header(const char* Header){
	appendToPageBuf("{name:\""); appendToPageBuf(Header); appendToPageBuf("\",labels:[],inputs:[],},");
}
void WT_SectionHeader(const char* Header){
	appendToPageBuf("{sectionHeader:\""); appendToPageBuf(Header); appendToPageBuf("\"},");
}
void WT_StartLine()       { appendToPageBuf("{"); }
void WT_EndLine()         { appendToPageBuf("},"); }
void WT_StartLabel()      { appendToPageBuf("labels:[{"); }
void WT_StartLabelEmpty() { appendToPageBuf("labels:[],"); }
void WT_StartInput()      { appendToPageBuf("inputs:[{"); }
void WT_StartInputEmpty() { appendToPageBuf("inputs:[],"); }
void WT_NextColumn()      { appendToPageBuf("},{"); }
void WT_EndCell()         { appendToPageBuf("}],"); }
void WT_TypeCell(const char* Type) { appendToPageBuf("type:\""); appendToPageBuf(Type); appendToPageBuf("\","); }
void WT_TextLabel(const char* Txt) { appendToPageBuf("text:\""); appendToPageBuf(Txt); appendToPageBuf("\","); }
void WT_ValueInput(const char* Val){ appendToPageBuf("value:\""); appendToPageBuf(Val); appendToPageBuf("\","); }
void WT_IDCell(const char* ID)     { appendToPageBuf("id:\""); appendToPageBuf(ID); appendToPageBuf("\","); }
void WT_AttrsStart()               { appendToPageBuf("attributes:["); }
void WT_Attr(const char* n, const char* v){
	appendToPageBuf("{n:\""); appendToPageBuf(n); appendToPageBuf("\",v:\""); appendToPageBuf(v); appendToPageBuf("\"},");
}
void WT_AttrsEnd()        { appendToPageBuf("],"); }
void WT_OptionsStart()    { appendToPageBuf("options:["); }
void WT_OptionsValues(const char* AVal){ appendToPageBuf("\""); appendToPageBuf(AVal); appendToPageBuf("\","); }
void WT_OptionsEnd()      { appendToPageBuf("],"); }

// ─── WT_Column: primary (PageVar*) ────────────────────────────────────────────
void WT_Column(int WT_Type, int WT_Begin, int WT_Finish, const char* Type, const char* Labval, const char* ID, const char* Style, PageVar* ListObj, const char* ExtraAttrs){
	if(WT_Type == WT_LABEL){
		if(!Labval[0]) { WT_StartLabelEmpty(); }
		else {
			if(WT_Begin == WT_FIRST) { WT_StartLabel(); }
			WT_TypeCell(Type);
			WT_TextLabel(Labval);
		}
	} else if(WT_Type == WT_INPUT){
		if(!Labval[0]) { WT_StartInputEmpty(); }
		else {
			if(WT_Begin == WT_FIRST) { WT_StartInput(); }
			WT_TypeCell(Type);
			WT_ValueInput(Labval);
		}
	}
	if(Labval[0]){
		WT_IDCell(ID);
		WT_AttrsStart();
		WT_Attr("style", Style);
		if(ExtraAttrs && ExtraAttrs[0]) { appendToPageBuf(ExtraAttrs); }
		if(ListObj && ListObj->Var){
			if(ListObj->Var->GetAttrTextAlign().length()){
				String ts = "text-align:"; ts += ListObj->Var->GetAttrTextAlign();
				WT_Attr("style", ts.c_str());
			}
			if(WT_Type == WT_INPUT && strcmp(Type,"select") && strcmp(Type,"checkbox")){
				if(ListObj->Var->HasAttrMin())  { char b[20]; snprintf(b,20,"%.6g",ListObj->Var->GetAttrMin());  WT_Attr("min",b); }
				if(ListObj->Var->HasAttrMax())  { char b[20]; snprintf(b,20,"%.6g",ListObj->Var->GetAttrMax());  WT_Attr("max",b); }
				if(ListObj->Var->HasAttrStep()) { char b[20]; snprintf(b,20,"%.6g",ListObj->Var->GetAttrStep()); WT_Attr("step",b); }
				if(ListObj->Var->GetAttrPlaceholder().length()) { WT_Attr("placeholder",ListObj->Var->GetAttrPlaceholder().c_str()); }
				if(ListObj->Var->GetAttrReadOnly()) { WT_Attr("readonly","true"); }
			}
		}
		WT_AttrsEnd();
		if(!strcmp(Type,"select")){
			WT_OptionsStart();
			int cnt = (ListObj && ListObj->Var) ? ListObj->Var->GetOptionCount() : 0;
			for(int i = 0; i < cnt; i++) { WT_OptionsValues(ListObj->Var->GetOption(i).c_str()); }
			WT_OptionsEnd();
		}
		if(WT_Finish == WT_MORE) { WT_NextColumn(); } else { WT_EndCell(); }
	}
}

// ─── WT_Column: compat overload (SelectOptions*) ──────────────────────────────
void WT_Column(int WT_Type, int WT_Begin, int WT_Finish, const char* Type, const char* Labval, const char* ID, const char* Style, SelectOptions* OptionList, int OptionListArraySize){
	if(WT_Type == WT_LABEL){
		if(!Labval[0]) { WT_StartLabelEmpty(); }
		else {
			if(WT_Begin == WT_FIRST) { WT_StartLabel(); }
			WT_TypeCell(Type);
			WT_TextLabel(Labval);
		}
	} else if(WT_Type == WT_INPUT){
		if(!Labval[0]) { WT_StartInputEmpty(); }
		else {
			if(WT_Begin == WT_FIRST) { WT_StartInput(); }
			WT_TypeCell(Type);
			WT_ValueInput(Labval);
		}
	}
	if(Labval[0]){
		WT_IDCell(ID);
		WT_AttrsStart();
		WT_Attr("style", Style);
		WT_AttrsEnd();
		if(OptionList != nullptr && OptionListArraySize > 0){
			WT_OptionsStart();
			for(int i = 0; i < OptionListArraySize; i++) { WT_OptionsValues(OptionList[i].Option.c_str()); }
			WT_OptionsEnd();
		}
		if(WT_Finish == WT_MORE) { WT_NextColumn(); } else { WT_EndCell(); }
	}
}

// ─── WT_AutoColumn ────────────────────────────────────────────────────────────
void WT_AutoColumn(int WT_Type, int WT_Begin, int WT_Finish, PageVar* pv, const char* IDPrefix, const char* ExtraAttrs){
	String type  = (WT_Type == WT_LABEL) ? "text" : pv->Var->GetTypeStr();
	String id    = pv->Var->GetID(IDPrefix);
	String val   = (WT_Type == WT_LABEL) ? pv->Var->GetDescription() : pv->Var->GetName();
	String style = (WT_Type == WT_LABEL) ? pv->Var->GetFStyle() : pv->Var->GetVarStyle();
	WT_Column(WT_Type, WT_Begin, WT_Finish, type.c_str(), val.c_str(), id.c_str(), style.c_str(), pv, ExtraAttrs);
}

// ─── WT_DisplayRow  — description label + value label, no input ───────────────
void WT_DisplayRow(PageVar* pv){
	appendToPageBuf("{labels:[{");
	WT_TypeCell("text");
	WT_TextLabel(pv->Var->GetDescription().c_str());
	WT_IDCell(pv->Var->GetID("L").c_str());
	WT_AttrsStart(); WT_Attr("style",pv->Var->GetFStyle().c_str()); WT_AttrsEnd();
	appendToPageBuf("},{");
	WT_TypeCell("text");
	WT_TextLabel(pv->Var->GetName().c_str());
	WT_IDCell(pv->Var->GetID("V").c_str());
	WT_AttrsStart(); WT_Attr("style",pv->Var->GetVarStyle().c_str()); WT_AttrsEnd();
	appendToPageBuf("}],inputs:[],},");
}

// ─── WT_EditRow  — description label + editable input ────────────────────────
void WT_EditRow(PageVar* pv, const char* typeOverride, const char* ExtraAttrs){
	String typeStr = (typeOverride && typeOverride[0]) ? typeOverride : pv->Var->GetTypeStr();
	appendToPageBuf("{labels:[{");
	WT_TypeCell("text");
	WT_TextLabel(pv->Var->GetDescription().c_str());
	WT_IDCell(pv->Var->GetID("L").c_str());
	WT_AttrsStart(); WT_Attr("style",pv->Var->GetFStyle().c_str()); WT_AttrsEnd();
	appendToPageBuf("}],inputs:[{");
	WT_TypeCell(typeStr.c_str());
	WT_ValueInput(pv->Var->GetName().c_str());
	WT_IDCell(pv->Var->GetID("").c_str());
	WT_AttrsStart();
	WT_Attr("style",pv->Var->GetFStyle().c_str());
	if(ExtraAttrs && ExtraAttrs[0]) { appendToPageBuf(ExtraAttrs); }
	if(pv->Var->GetAttrTextAlign().length()){
		String ts = "text-align:"; ts += pv->Var->GetAttrTextAlign();
		WT_Attr("style",ts.c_str());
	}
	if(typeStr != "select" && typeStr != "checkbox"){
		if(pv->Var->HasAttrMin())  { char b[20]; snprintf(b,20,"%.6g",pv->Var->GetAttrMin());  WT_Attr("min",b); }
		if(pv->Var->HasAttrMax())  { char b[20]; snprintf(b,20,"%.6g",pv->Var->GetAttrMax());  WT_Attr("max",b); }
		if(pv->Var->HasAttrStep()) { char b[20]; snprintf(b,20,"%.6g",pv->Var->GetAttrStep()); WT_Attr("step",b); }
		if(pv->Var->GetAttrPlaceholder().length()) { WT_Attr("placeholder",pv->Var->GetAttrPlaceholder().c_str()); }
		if(pv->Var->GetAttrReadOnly()) { WT_Attr("readonly","true"); }
	}
	if(typeStr == "select"){
		WT_AttrsEnd();
		WT_OptionsStart();
		for(int i = 0; i < pv->Var->GetOptionCount(); i++) { WT_OptionsValues(pv->Var->GetOption(i).c_str()); }
		WT_OptionsEnd();
	} else {
		WT_AttrsEnd();
	}
	appendToPageBuf("}],},");
}
// ─── WT_CheckEditRow — label, then tick box + field together in one cell ─────
// e.g. "Nightly Reboot at  [x] [00:05]".  Both keep their own id, so UPDATE and
// the live websocket updates work exactly as for two separate rows.
static void WT_InputObj(PageVar* pv, const char* type, const char* extraStyle){
	appendToPageBuf("{");
	WT_TypeCell(type);
	WT_ValueInput(pv->Var->GetName().c_str());
	WT_IDCell(pv->Var->GetID("").c_str());
	WT_AttrsStart();
	String st = pv->Var->GetFStyle(); st += extraStyle;
	WT_Attr("style", st.c_str());
	WT_AttrsEnd();
	appendToPageBuf("}");
}
void WT_CheckEditRow(PageVar* chk, PageVar* edit, const char* editType){
	String et = (editType && editType[0]) ? String(editType) : edit->Var->GetTypeStr();
	appendToPageBuf("{grouped:true,labels:[{");
	WT_TypeCell("text");
	WT_TextLabel(chk->Var->GetDescription().c_str());
	WT_IDCell(chk->Var->GetID("L").c_str());
	WT_AttrsStart(); WT_Attr("style",chk->Var->GetFStyle().c_str()); WT_AttrsEnd();
	appendToPageBuf("}],inputs:[");
	WT_InputObj(chk, "checkbox", ";vertical-align:middle");
	appendToPageBuf(",");
	WT_InputObj(edit, et.c_str(), ";vertical-align:middle;margin-left:6px");
	appendToPageBuf("],},");
}
////// END - Page generation for a table //////

//  options: ["SYSTEM A", "NEW SYSTEM", "OLD SYSTEM"],