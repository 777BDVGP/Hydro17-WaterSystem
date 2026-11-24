
// =============================================================================
// 12_Settings.cpp — AllParams Class Implementation & Settings Page
// =============================================================================
// Implements the AllParams class (JSON persistence to /AllParams.json), the
// /settings page handler, and the PageVar map for the settings form.
// MaintainSettingsPageValues() detects changes and pushes them to browsers.
// =============================================================================

// ─── AllParams Class ──────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// CreateAllParams
// Allocates the global AllParams instance, loads saved values from SPIFFS,
// then builds the Settings page-variable map.  Called from InitOperationalObjects().
// ─────────────────────────────────────────────────────────────────────────────
 void CreateAllParams() {
	// Create AllParams Object
    Params = new AllParams;
    Params->ReadData();	
	Params->WriteData(); 	
	
	MapSettingsPage();    
}	

AllParams::AllParams()  {}        // CONSTRUCTOR
AllParams::~AllParams(){}         // DESTRUCTOR
void AllParams::Maintain(){}

void AllParams::WriteData()
{
	String StrFileName = "/AllParams.json";	
	File ChannelDataFile = SPIFFS.open(StrFileName, "w");
	JsonDocument doc; 

	doc["SysName"] = SysName;    
	doc["LogGeneralEvents"] = LogGeneralEvents;
	doc["LogIrrigationEvents"] = LogIrrigationEvents;
	doc["LogTankFillingEvents"] = LogTankFillingEvents;	
	doc["WatchdogTime"] = WatchdogTime;	
	doc["ResetTimeOnContDisabled"] = ResetTimeOnContDisabled;	
	doc["NormalOperationSaved"] = NormalOperationSaved;
	doc["KeepAliveEnabled"] = KeepAliveEnabled;
	doc["MidnightRebootEnabled"] = MidnightRebootEnabled;
	doc["RainGaugeEnable"] = RainGaugeEnable;
	doc["RainGaugeURL"] = RainGaugeURL;
	doc["ShowOtaInfo"] = ShowOtaInfo;
	
	if (ChannelDataFile) 
	{   
		serializeJsonPretty(doc, ChannelDataFile);  
		ChannelDataFile.close();
	}	

}

void AllParams::ReadData()
{
	String StrFileName = "/AllParams.json";	
	File ChannelDataFile = SPIFFS.open(StrFileName, "r");	
	if (ChannelDataFile) 
	{ 	
		size_t size = ChannelDataFile.size();    
		if (size <= 1200)
		{
			std::unique_ptr<char[]> buf(new char[size]); // Buffer to store file contents
			ChannelDataFile.readBytes(buf.get(), size);
			JsonDocument doc; 
			auto error = deserializeJson(doc, buf.get());	
			if (!error) 
			{		
				if ( doc["SysName"].as<String>() != "null" ){ SysName = doc["SysName"].as<String>();}					
				if ( doc["LogGeneralEvents"].as<String>() != "null" ) { LogGeneralEvents = doc["LogGeneralEvents"]; }					
				if ( doc["LogIrrigationEvents"].as<String>() != "null" ) { LogIrrigationEvents = doc["LogIrrigationEvents"]; }		
				if ( doc["LogTankFillingEvents"].as<String>() != "null" ) { LogTankFillingEvents = doc["LogTankFillingEvents"]; }						
				if ( doc["WatchdogTime"].as<String>() != "null" ) { WatchdogTime = doc["WatchdogTime"]; }		
				if ( doc["ResetTimeOnContDisabled"].as<String>() != "null" ) { ResetTimeOnContDisabled = doc["ResetTimeOnContDisabled"]; }		
				if ( doc["NormalOperationSaved"].as<String>() != "null" ){if(doc["NormalOperationSaved"].as<String>() == "true") {NormalOperationSaved = true;}else{NormalOperationSaved = false;}}	 				
				if ( !doc["KeepAliveEnabled"].isNull() ) { KeepAliveEnabled = doc["KeepAliveEnabled"].as<bool>(); }
				if ( !doc["MidnightRebootEnabled"].isNull() ) { MidnightRebootEnabled = doc["MidnightRebootEnabled"].as<bool>(); }
				if ( !doc["RainGaugeEnable"].isNull() ) { RainGaugeEnable = doc["RainGaugeEnable"].as<bool>(); }
				if ( !doc["RainGaugeURL"].isNull() )    { RainGaugeURL    = doc["RainGaugeURL"].as<String>(); }
				if ( !doc["ShowOtaInfo"].isNull() )     { ShowOtaInfo     = doc["ShowOtaInfo"].as<bool>(); }
			}
			else
			{
				SaveGenReport F(" File Error open File AllParams.json");  				
			}		
		}
		else
		{
			SaveGenReport F(" File AllParams.json size is > 1200 ");    
		}
		ChannelDataFile.close();  	
	}
}
 
/* class AllParams - END --------------------------------------------- */

void MapSettingsPage(){	
//  In this routine a list, SettingsList, is setup to connect the Webpage and the Settings Variables	
	SettingsList = new PageVar[PAGE_VAR_SETTINGS];
	for (int i=0; i < PAGE_VAR_SETTINGS; i++)
	{ 
		switch(i)    
		{ 	
			case SET_SYSNAME: 	
				SettingsList[i].Var = new PageElement(&Params->SysName, F("System Name"), "", &SetStyle, i+ID_SETTINGS);  
				break;
			case SET_LOG_GENERAL:
				SettingsList[i].Var = new PageElement(&Params->LogGeneralEvents, F("Max General Log Events"), "", &SetStyle, i+ID_SETTINGS); 
				break;				
			case SET_LOG_IRRIGATION:
				SettingsList[i].Var = new PageElement(&Params->LogIrrigationEvents, F("Max Irrigation Log Events"), "", &SetStyle, i+ID_SETTINGS);  		
				break;		
			case SET_LOG_TANKFILLING:
				SettingsList[i].Var = new PageElement(&Params->LogTankFillingEvents, F("Max Tank Filling Log Events"), "", &SetStyle, i+ID_SETTINGS);  				
				break;	
			case SET_WATCHDOG_TIME:
				SettingsList[i].Var = new PageElement(&Params->WatchdogTime, F("Watchdog to Reboot (Sec)"), "", &SetStyle, i+ID_SETTINGS);  
				break;		
			case SET_CONT_DISABLE_RESET:
				SettingsList[i].Var = new PageElement(&Params->ResetTimeOnContDisabled	, F("Enable Normal Operation (Min)"), "", &SetStyle, i+ID_SETTINGS);
				break;
			case SET_KEEPALIVE_ENABLE:
				SettingsList[i].Var = new PageElement(&Params->KeepAliveEnabled, F("Keepalive Ping Enabled"), "", &SetStyle, i+ID_SETTINGS);
				break;
			case SET_MIDNIGHT_REBOOT:
				SettingsList[i].Var = new PageElement(&Params->MidnightRebootEnabled, F("Midnight Reboot (00:05)"), "", &SetStyle, i+ID_SETTINGS);
				break;
			case SET_RAINGAUGE_EN:
				SettingsList[i].Var = new PageElement(&Params->RainGaugeEnable, F("Use Rain Gauge"), "", &SetStyle, i+ID_SETTINGS);
				break;
			case SET_RAINGAUGE_URL:
				SettingsList[i].Var = new PageElement(&Params->RainGaugeURL, F("Rain Gauge IP"), "", &SetStyle, i+ID_SETTINGS);
				break;
			case SET_SHOW_OTA:
				SettingsList[i].Var = new PageElement(&Params->ShowOtaInfo, F("Show PC Upload (OTA) on ABOUT"), "", &SetStyle, i+ID_SETTINGS);
				break;
		}
	}
}

void handleSettings(AsyncWebServerRequest *request){
	//SaveGenReport("handleSettings " + String(ESP.getFreeHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(Delmess, response);
	append_UL_OptionsMenu(PS_SETTINGS, response);
	response->print(webScriptTag);
	WT_SETUP_Settings(response);
	appendRainGaugeBlock(response);        // live rain-gauge badge + status (17_RainGauge.cpp)
	response->print(webUpateButton);
	appendWifiNetsSection(response);      // WIFI NETWORKS list + ADD, like the Rain Gauge (05_WiFi.cpp)
	response->print(webHTMLEnd);
	request->send(response);
}

void WT_SETUP_Settings(AsyncResponseStream* response){
// 	Create List of Setting Page Parameters 	
	resetPageBuf();
	WT_StartPage();
 	WT_StartTable("Parameter  ", "Value  ", "");

	for (int i=0; i < PAGE_VAR_SETTINGS; i++)
	{
		WT_EditRow(&SettingsList[i]);
	}

	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

void MaintainSettingsPageValues(){	
	for (int i=0; i < PAGE_VAR_SETTINGS; i++)
	{ 
		if(SettingsList[i].Var->GetName() != SettingsList[i].Var->GetNameLast())
		{
			webParamUpdate(SettingsList[i].Var->GetName(),SettingsList[i].Var->GetID(""),SettingsList[i].Var->GetVarStyle());SettingsList[i].Var->UpdateLast();							
		}
	}	
	SendRainGaugeValues();   // rain-gauge badge / status (17_RainGauge.cpp)
}

 void UpdateSettingsPageValues(uint32_t num, JsonObject *root){
	for (JsonPair kv : *root){	
//		Serial.print("Num: ");Serial.print(num); 		
//		Serial.print(" Key: ");Serial.print(kv.key().c_str()); 
//		Serial.print(" Value: ");Serial.println(kv.value().as<const char*>()); 	
	
		for (int i=0; i < PAGE_VAR_SETTINGS; i++)
		{ 
			if((SettingsList[i].Var->GetID("") == kv.key().c_str()))				
			{	
				SettingsList[i].Var->SetName(String(kv.value().as<const char*>()));	 
			}	
		}	
	}
	WebMessUpdate(num, "Parameters Updated!", "color: red", true);
	Params->WriteData();
	UpdateTimerTimes();
	MaintainDueDatesValues();
	GatewayWatchRestart();   // Apply keepalive enable change without a reboot
	RainGaugePollNow = true; // ask the rain gauge at once with the new IP / checkbox (17_RainGauge.cpp)
 }

// webNtfyTestButton and handleTestNtfy have moved to 16_NTFY.cpp.

// ─── WiFi Networks ────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// webWifiEditButton
// Dark-green "WiFi Networks" button placed below UPDATE on the Settings page.
// Navigates to /wifi (handleWifiNets, 05_WiFi.cpp): list / add / delete.
// ─────────────────────────────────────────────────────────────────────────────
void webWifiEditButton(AsyncResponseStream* r)
{
	r->print(F("<div><br></div><div><b><a>"
	           "<b><a><input type=\"button\" id=\"WIFIEDIT\" "
	           "onclick=\"window.location.href='/wifi'\" "
	           "style=\"font-size:14px;background-color:#166534;color:#fff;"
	           "border:none;padding:6px 14px;border-radius:4px;cursor:pointer\" "
	           "value=\"&#9881; WiFi Networks\"</a></b></div>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRemoveWifi  (GET /removewifi)
// Deletes the saved WiFi networks and reboots.  A private build re-joins its
// built-in networks; otherwise the Hydro17-Setup hotspot opens.
// ─────────────────────────────────────────────────────────────────────────────
void handleRemoveWifi(AsyncWebServerRequest *request) {
	sendHtmlPage(request, "<html><body><h3>Saved WiFi networks removed. Rebooting...</h3></body></html>");
	delay(200);
	ClearWifiSettings();
	SaveGenReport("REBOOT - Saved WiFi networks removed via /removewifi");
	delay(100);
	ESP.restart();
}
