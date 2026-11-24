
// =============================================================================
// 05_ThisSystem.cpp — MaintainSystem, I/O Setup, Debounce & Web Routes
// =============================================================================
// Implements the 10 ms MaintainSystem that drives all physical control logic,
// the I/O initialisation and debounce routines, the web page maintenance
// dispatcher, and the AddRoutes() function that maps all HTTP endpoints.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// MaintainSystem  (called every 10 ms from loop(), main.cpp)
// Drives all physical control logic.  loop() skips it while an OTA update is
// in progress so the network stack is not competed with.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainSystem() {
	MaintainValveStatus(false);   // Valve relay state machine        (09_WaterValves.cpp)
	IrrigationShedule();          // Irrigation zone scheduler         (09_Irrigation.cpp)
	MaintainTankFilling();        // Tank level & pump management      (12_Tank.cpp)
	MaintainController();         // System enable/disable controller  (11_Other.cpp)
	MaintainPoolFilterPump();     // Pool filter pump scheduler        (13_Pool.cpp)
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupSystemIOs
// Configures all output pins as OUTPUT (LOW) and all input pins as INPUT.
// Called from InitHardware() during setup() before WiFi provisioning.
// ─────────────────────────────────────────────────────────────────────────────
void SetupSystemIOs()
{

	for (int i=0; i < OutPorts; i++){ 
	    OutPort[i].OUTStat = false;   //UPD_19 
		pinMode(OutPort[i].Port, OUTPUT);
		digitalWrite(OutPort[i].Port, LOW);  
	}

	for (int i=0; i < InputPorts; i++)
	{ 
		pinMode(InPort[i].PinIO, INPUT);
	}           
}

// ─────────────────────────────────────────────────────────────────────────────
// PinDebounce  (called every 10 ms from MaintainTimeCheck)
// Iterates over all InPort[] entries and applies a 50 ms steady-state filter.
// A rising edge must hold HIGH for DEBOUNCE_STEADY_MS before Input goes true;
// a falling edge must hold LOW for the same period before Input goes false.
// ─────────────────────────────────────────────────────────────────────────────
void PinDebounce(){ // From MaintainTimeCheck
	for (int i=0; i < InputPorts; i++)
	{     	
		// Normal switch Input		
		bool Reading = digitalRead(InPort[i].PinIO);	 

		DebouceTimeRead = millis(); 		
		if ( Reading && !InPort[i].Input ) // Was not Triggered Before && Input = Low
		{
			InPort[i].MemoryL = false; 				
			if(!InPort[i].MemoryH)
			{
				InPort[i].Time = DebouceTimeRead;
				InPort[i].MemoryH = true; 
			}
			else if( DebouceTimeRead < InPort[i].Time || DebouceTimeRead < DEBOUNCE_STEADY_MS ) // DebouceTime Valid 
			{
				InPort[i].MemoryH = false;
			}				
			else if(( DebouceTimeRead - InPort[i].Time ) > DEBOUNCE_STEADY_MS ) // Steady for DEBOUNCE_STEADY_MS
			{
				InPort[i].Input = true;
			}
		}
		else if( !Reading )
		{
			InPort[i].MemoryH = false; 		
			if(InPort[i].Input) 
			{
				if(!InPort[i].MemoryL) // Was not Triggered Before
				{
					InPort[i].Time = DebouceTimeRead;
					InPort[i].MemoryL = true; 
				}				
				else if( DebouceTimeRead < InPort[i].Time || DebouceTimeRead < DEBOUNCE_STEADY_MS ) // DebouceTime Valid
				{
					InPort[i].MemoryL = false;
				}				
				else if(( DebouceTimeRead - InPort[i].Time ) > DEBOUNCE_STEADY_MS ) // Steady for DEBOUNCE_STEADY_MS  
				{								
					InPort[i].Input = false;							
				} 	
			}
		}	
	}	
}

// ─────────────────────────────────────────────────────────────────────────────
// MaintainWebPageValues  (called from MaintainWebService via MaintainWebPage)
// Dispatches to each active page's maintenance function so that changed
// variable values are detected and pushed to the browser via WebSocket.
// Pages are only maintained if their PageVar list has been allocated (Object flag = true).
// ─────────────────────────────────────────────────────────────────────────────
void MaintainWebPageValues(){ // From MaintainWebService

	MaintainSettingsPageValues();    // This is a copy of the Borehole Controller SETTINGS
	if(NtfyPageObject) MaintainNtfyPageValues();
	if(TankPageObject) MaintainTankPageValues();
	if(MainPageObject) MaintainHomePageValues();
    if(SuspendObject)  MaintainSuspendPageValues();
	SendFwValues();                  // ABOUT firmware update status (22_Update.cpp)
}

// ─────────────────────────────────────────────────────────────────────────────
// AddRoutes
// Registers all HTTP GET/POST route handlers with the AsyncWebServer.
// Called once from SetupWebPage() in 04_WebPages.cpp.
// Routes are grouped: irrigation, tank/pool/plugs, manual/reports/settings,
// about/OTA, and file management (files/download/upload/delete).
// ─────────────────────────────────────────────────────────────────────────────
void AddRoutes(){
	server.on("/",                   handleRoot);             	// 08_Home.cpp         -  MainPageObject
	server.on("/irrigation",         handleIrrigation);       	// 09_Irrigation.cpp
	server.on("/zonesnext",          handleIrrigSetupNext);     // 09_Irrigation.cpp
	server.on("/zonesprev",          handleIrrigSetupPrev);     // 09_Irrigation.cpp
	server.on("/resumeirrigateion",  handleResumeIrrig);		// 09_Irrigation.cpp
	server.on("/suspendirrigateion", handleSuspendIrrig);       // 09_Irrigation.cpp
	server.on("/managesuspend",      handleManageSuspend);      // 09_Suspend.cpp      -  SuspendObject
	server.on("/valvenames",         handleWaterValves);        // 09_WaterValves.cpp  -  ElectricValvesObject
	server.on("/valveconfigs",       handleValvesConfigs);      // 09_WaterValves.cpp  -  ElectricValvesObject

	server.on("/tank",               handleTank);		        // 12_Tank.cpp
	server.on("/starttankfill",      handleStartTankFill);      // 12_Tank.cpp
	server.on("/stoptankfill",       handleStopTankFill);       // 12_Tank.cpp
	server.on("/pool",               handlePool);		        // 13_Pool.cpp
	server.on("/switchonPool",       handlePoolOn);		        // 13_Pool.cpp
	server.on("/switchoffPool",      handlePoolOff);            // 13_Pool.cpp
	server.on("/poollight",          handlePoolLight);          // 13_Pool.cpp
	server.on("/switchonlight",      handleLightOn);		    // 13_Pool.cpp
	server.on("/switchofflight",     handleLigthOff);           // 13_Pool.cpp
	server.on("/switchoffManLight",  handleLightManOff);        // 13_Pool.cpp
	server.on("/plugs",              handlePlugs);		        // 14_Plugs.cpp

	server.on("/other",              handleOptions);	        // 11_Other.cpp
	server.on("/reportgen",          handleGenReport);          // 11_Other.cpp
	server.on("/reportpump",         handleIrrigReport);        // 11_Other.cpp
	server.on("/reportfill",         handleTfillReport);        // 11_Other.cpp
	server.on("/settings",           handleSettings);	        // 12_Settings.cpp
	server.on("/wifi",               handleWifiNets);           // 05_WiFi.cpp — WiFi Networks page
	server.on("/wifinet/add",    HTTP_POST, handleWifiNetAdd);    // 05_WiFi.cpp
	server.on("/wifinet/delete", HTTP_POST, handleWifiNetDelete); // 05_WiFi.cpp
	server.on("/removewifi",         handleRemoveWifi);         // 12_Settings.cpp
	server.on("/ntfy",               handleNtfy);               // 16_NTFY.cpp
	server.on("/applydate",          handleApplyDate);          // 16_NTFY.cpp
	server.on("/testntfy",           handleTestNtfy);           // 16_NTFY.cpp

	server.on("/discontroller",      handleManualOperContDis);	// 11_Other.cpp
	server.on("/enablecontroller",   handleManualOperContEna);  // 11_Other.cpp

	server.on("/reports",            handleReports);            // 11_Other.cpp

	server.on("/about",              handleAbout);              // 15_About.cpp
	server.on("/setASP",             handleSetASP);             // 15_About.cpp
	server.on("/setReboot",          handleSetReboot);          // 15_About.cpp
	server.on("/fwcheck",    HTTP_POST, handleFwCheck);          // ABOUT "CHECK FOR UPDATE" (22_Update.cpp)
	server.on("/fwinstall",  HTTP_POST, handleFwInstall);        // ABOUT "INSTALL FROM GITHUB"
	server.on("/update",     HTTP_POST, handleFwUploadDone, handleFwUploadData);  // ABOUT ".bin upload"

//  Files — specific sub-paths must come before "/files" because
//  ESPAsyncWebServer 3.x prefix-matches and "/files" would capture "/files/*".
	server.on("/files/view",         handleFileView);
	server.on("/files/delete",       handleFileDeleteJson);
	server.on("/api/files",          handleApiFiles);
	server.on("/files",              FilesPage);
	server.on("/download",           File_Download);
	server.on("/upload",             File_Upload);
	server.on("/fupload", HTTP_POST,
		[](AsyncWebServerRequest *request){ request->redirect("/files"); },
		handleFileUpload);
	server.on("/delete",             File_Delete);
}






	
