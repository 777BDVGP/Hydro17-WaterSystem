// =============================================================================
// 00_ForwardDecl.h — Forward Declarations for All Module Functions
// =============================================================================
// Provides explicit function prototypes for every function defined across all
// .cpp module files.  In the Arduino IDE these are auto-generated; in
// PlatformIO (single-TU build via #include in main.cpp) they must be declared
// explicitly before the first call site.
// Must be included AFTER all library/header files (01_Main.h etc.) are already
// included so that parameter types (AsyncWebServerRequest, PageVar, etc.) are
// visible to the compiler when it processes these declarations.
// =============================================================================

// ─── FreeRTOS Task Handles ────────────────────────────────────────────────────
extern TaskHandle_t NtfyTaskTaskHandle;

// ─── loop() Workers & FreeRTOS Task Functions ─────────────────────────────────
void MaintainSystem();                   // 05_ThisSystem.cpp — loop(), 10 ms
void MaintainTimeCheck();                // 02_Time.cpp       — loop(), 10 ms
void MaintainWebService();               // 04_WebPages.cpp   — loop(), every pass
void NtfyTask(void *pvParameters);       // 16_NTFY.cpp

// --- 01_Main.cpp ---
void InitHardware();
void WatchdogSetup();
void ISRwatchdog();
void LEDToggle();
void ISR_LED_Setup();
void MaintainGatewayWatch();   // Keepalive ping + reboot safety net — do NOT remove
void GatewayWatchRestart();

// --- 02_Time.cpp ---
void SetupTime();
void MaintainTimers();
void MaintainMidnightReboot();
void UpdateWebPageTime();
void CompileDateTime();
void ReduceFiles();
void reduce_file(String filename, int Entries);
void activateTimer(int index);
void deactivateTimer(int index);
bool TimerExpired(int index);
void SetValueAndAtivate(String Units, int TimerUnits, int index);
void SetTimerValue(int Units, int index);
int  RemainingSec(int index);
int  RemainingMin(int index);
String IncrementDateWithOneDay(String Date);
String AddTimeToCurrentDateTime(String Date, String Time);
int  MonthDaysValue(int Year, int Month);
void DetermineWeekday();
String GetCurrentDateAndTime();
String GetCurrentDate(int DaysAdded);
String GetCurrentTime();
String IntToStringYearMonthDay(int Year, int Month, int Day);
String IntToStringHourMinute(int Hour, int Minute);
String GetHrMinSec(int Sec, bool RetHours);
String GetDifferenceTimeStartBusy(String TimeStart, String TimeBusy);
void UpdateTimerTimes();
int  GetDifferenceBetweenTwoDates(String StartDate, String EndDate);

// --- 05_WiFi.cpp ---
bool SetupWiFi();
void ClearWifiSettings();
void handleWifiNets(AsyncWebServerRequest *request);
void appendWifiNetsSection(AsyncResponseStream* r);
void handleWifiScan(AsyncWebServerRequest *request);
void handleWifiNetAdd(AsyncWebServerRequest *request);
void handleWifiNetDelete(AsyncWebServerRequest *request);
void MaintainWifiNets();

// --- 17_RainGauge.cpp ---
void SetupRainGauge();
void MaintainRainGauge();
void SendRainGaugeValues();
void appendRainGaugeBlock(AsyncResponseStream* r);

// --- 22_Update.cpp ---
void handleFwCheck(AsyncWebServerRequest* r);
void handleFwInstall(AsyncWebServerRequest* r);
void handleFwUploadData(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
void handleFwUploadDone(AsyncWebServerRequest* request);
void MaintainFwUpdate();
void SendFwValues();
void appendFwUpdateBlock(AsyncResponseStream* r);

// --- 03_Files.cpp ---
void InitFileSystem();
void InitOperationalObjects();
void InitOther();
void FilesPage(AsyncWebServerRequest *request);
void FilePageSel(int Index, AsyncResponseStream* r);
void handleApiFiles(AsyncWebServerRequest *request);
void handleFileView(AsyncWebServerRequest *request);
void handleFileDeleteJson(AsyncWebServerRequest *request);
void File_Download(AsyncWebServerRequest *request);
void CompilePage_Download(AsyncWebServerRequest *request);
void append_Form_Download(AsyncResponseStream* r);
void DownloadFile(AsyncWebServerRequest *request, String filename);
void File_Upload(AsyncWebServerRequest *request);
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void File_Delete(AsyncWebServerRequest *request);
void Compile_PageDelete(AsyncWebServerRequest *request);
void append_Form_Delete(AsyncResponseStream* r);
void SPIFFS_file_delete(AsyncWebServerRequest *request, String filename);
void CreateDFileList(const char* dir);

// --- 04_WebPages.cpp ---
void SetupWebPage();
void MaintainWebPage();
void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webParamUpdate(const String& Name, const String& ID, const String& ATT, const char* ATTName = "style");
void UnpackJasonPayload(uint32_t num, uint8_t* payload, size_t length);
void append_FileOption(int ReqMess, AsyncResponseStream* r);
void WebMessUpdate(uint32_t num, const String& Mess, const String& Att, bool Reset);
void UpdateSelectMessDelay();
void WT_StartTable(const char* Hdr1, const char* Hdr2, const char* Hdr3);
void WT_Column(int WT_Type, int WT_Begin, int WT_Finish, const char* Type, const char* Labval, const char* ID, const char* Style, PageVar* ListObj, const char* ExtraAttrs = "");
void WT_Column(int WT_Type, int WT_Begin, int WT_Finish, const char* Type, const char* Labval, const char* ID, const char* Style, SelectOptions* OptionList, int OptionListArraySize);
void WT_AutoColumn(int WT_Type, int WT_Begin, int WT_Finish, PageVar* pv, const char* IDPrefix = "", const char* ExtraAttrs = "");
void WT_DisplayRow(PageVar* pv);
void WT_EditRow(PageVar* pv, const char* typeOverride = "", const char* ExtraAttrs = "");
void WT_CheckEditRow(PageVar* chk, PageVar* edit, const char* editType = "");
void WT_StartPage();
void WT_EndPage();
void WT_EndPageClass(const char* cls);
void WT_Header(const char* Header);
void WT_SectionHeader(const char* Header);
void WT_StartLine(); void WT_EndLine();
void WT_StartLabel(); void WT_StartLabelEmpty();
void WT_StartInput(); void WT_StartInputEmpty();
void WT_NextColumn(); void WT_EndCell();
void WT_TypeCell(const char* Type);
void WT_TextLabel(const char* Txt);
void WT_ValueInput(const char* Val);
void WT_IDCell(const char* ID);
void WT_AttrsStart();
void WT_Attr(const char* n, const char* v);
void WT_AttrsEnd();
void WT_OptionsStart();
void WT_OptionsValues(const char* AVal);
void WT_OptionsEnd();

// --- 05_ThisSystem.cpp ---
void SetupSystemIOs();
void PinDebounce();
void MaintainWebPageValues();
void AddRoutes();
void appendMainPageSel(int Index, AsyncResponseStream* r);

// --- 08_Home.cpp ---
void handleRoot(AsyncWebServerRequest *request);
void MapHomePage();
void WT_HomePage(AsyncResponseStream* response);
void MaintainHomePageValues();
void MaintainHomePage();
void ForceHomeIrrigatePush();
void MaintainDueDatesValues();
void webRebootButton(AsyncResponseStream* r);

// --- 09_Irrigation.cpp ---
void handleIrrigation(AsyncWebServerRequest *request);
void IrrigationPage(AsyncWebServerRequest *request);
void CompilePage_Irrigation(AsyncWebServerRequest *request);
void IrrigatePageSel(int Index, AsyncResponseStream* r);
void append_Form_Irrigation(AsyncResponseStream* response);
void UpdateButtons();
void handleIrrigSetupNext(AsyncWebServerRequest *request);
void handleIrrigSetupPrev(AsyncWebServerRequest *request);
void CompilePage_IrrigSetup(AsyncWebServerRequest *request, int Line);
void Irrigate_Next_Prev(AsyncResponseStream* r);
void WT_SETUP_IrrigSetup(AsyncResponseStream* response, int Line);
void UpdateIrrigSetupPageValues(uint32_t num, JsonObject* root);
void handleResumeIrrig(AsyncWebServerRequest *request);
void handleSuspendIrrig(AsyncWebServerRequest *request);
void CreateIrrigation();
void CreateSheduleObj();
String GetScheduledDateAndTime(int Entry);
void IrrigationInit();
void UpdateToNextDate(int Entry);
void IrrigationShedule();
void SuspendAllIrrigation();
void SetupIrrigateShedule(int Entry, String Type);

// --- 09_Suspend.cpp ---
void handleManageSuspend(AsyncWebServerRequest *request);
void CompilePage_Suspend(AsyncWebServerRequest *request);
void MapSuspendPage();
void WT_SETUP_Suspend(AsyncResponseStream* response);
void MaintainSuspendPageValues();
void UpdateSuspendPageValues(uint32_t num, JsonObject* root);
void CreateSuspendDays();
void CreateSuspendMaps();
void MaintainSuspention(bool isMidnight);
void RecordSuspendIrriSet(String Msg);

// --- 09_WaterValves.cpp ---
void handleWaterValves(AsyncWebServerRequest *request);
void WT_SETUP_WaterValves(AsyncResponseStream* response);
void UpdateWaterValvesPageValues(uint32_t num, JsonObject* root);
void handleValvesConfigs(AsyncWebServerRequest *request);
void WT_SETUP_Config(AsyncResponseStream* response);
void UpdateConfigPageValues(uint32_t num, JsonObject* root);
void CreateElectricValves();
void ActivateValves(int Config, int ValveState);
void MaintainValveStatus(bool delaySwitch);

// --- 11_Other.cpp ---
void handleManualOperContDis(AsyncWebServerRequest *request);
void handleManualOperContEna(AsyncWebServerRequest *request);
void handleOptions(AsyncWebServerRequest *request);
void append_UL_OptionsMenu(int Index, AsyncResponseStream* r);
void handleReports(AsyncWebServerRequest *request);
void append_UL_ReportsMenu(int Index, AsyncResponseStream* r);
void CreateButton();
void InitFor_NormalOperation();
void InitFor_NOT_NormalOperation();
void MaintainController();
void SwitchNormalOperation();
void SetMessColour(String Mess, String Color);
void handleGenReport(AsyncWebServerRequest *request);
void CompilePage_ViewGeneralReport(AsyncWebServerRequest *request);
void append_Form_ViewGeneralReport(AsyncResponseStream* response);
void SaveGenReport(String p_entry);
void handleIrrigReport(AsyncWebServerRequest *request);
void CompilePage_ViewIrrigReport(AsyncWebServerRequest *request);
void append_Form_ViewIrrigReport(AsyncResponseStream* response);
void SaveIrrigReport(String p_entry);
void handleTfillReport(AsyncWebServerRequest *request);
void CompilePage_ViewTfillReport(AsyncWebServerRequest *request);
void append_Form_ViewTfillReport(AsyncResponseStream* response);
void SaveTankFillReport(String p_entry);

// --- 12_Settings.cpp ---
void CreateAllParams();
void MapSettingsPage();
void handleSettings(AsyncWebServerRequest *request);
void WT_SETUP_Settings(AsyncResponseStream* response);
void MaintainSettingsPageValues();
void UpdateSettingsPageValues(uint32_t num, JsonObject* root);
void webWifiEditButton(AsyncResponseStream* r);
void handleRemoveWifi(AsyncWebServerRequest *request);

// --- 16_NTFY.cpp ---
void CreateNtfyParams();
void MapNtfyPage();
void handleNtfy(AsyncWebServerRequest *request);
void handleApplyDate(AsyncWebServerRequest *request);
void WT_ApplyDatesTable(AsyncResponseStream* response);
void WT_SETUP_Ntfy(AsyncResponseStream* response);
void MaintainNtfyPageValues();
void UpdateNtfyPageValues(uint32_t num, JsonObject *root);
void webNtfyTestButton(AsyncResponseStream* r);
void handleTestNtfy(AsyncWebServerRequest *request);
void SetupNTFY();
void NtfyTask(void *pvParameters);
void MessageNTFY(String message, String priority);
void MaintainNTFY();

// --- 12_Tank.cpp ---
void handleTank(AsyncWebServerRequest *request);
void handleStartTankFill(AsyncWebServerRequest *request);
void handleStopTankFill(AsyncWebServerRequest *request);
void TankPageSel(int Index, AsyncResponseStream* r);
void CompilePage_Tank(AsyncWebServerRequest *request);
void append_Form_Tank(AsyncResponseStream* response);
void MapTankPage();
void WT_SETUP_Tank(AsyncResponseStream* response);
void MaintainTankPageValues();
void UpdateTankPageValues(uint32_t num, JsonObject* root);
void CreatePressureSensors();
void ActivateTankFilterTimer();
void MaintainTankFilling();

// --- 13_Pool.cpp ---
void handlePoolOn(AsyncWebServerRequest *request);
void handlePoolOff(AsyncWebServerRequest *request);
void handleLightOn(AsyncWebServerRequest *request);
void handleLigthOff(AsyncWebServerRequest *request);
void handleLightManOff(AsyncWebServerRequest *request);
void handlePool(AsyncWebServerRequest *request);
void append_UL_PoolMenu(AsyncResponseStream* r);
void WT_PoolPage(AsyncResponseStream* response);
String SetBool(bool Val);
void UpdatePoolPageValues(uint32_t num, JsonObject* root);
void CreatePoolShedule();
void handlePoolLight(AsyncWebServerRequest *request);
void append_UL_PoolLightMenu(AsyncResponseStream* r);
void WT_PoolLightPage(AsyncResponseStream* response);
void UpdatePoolLightPageValues(uint32_t num, JsonObject* root);
void CreatePoolLightShedule();
void MaintainPoolFilterPump();

// --- 14_Plugs.cpp ---
void handlePlugs(AsyncWebServerRequest *request);
void CompilePage_Plugs(AsyncWebServerRequest *request);
void append_Form_Plugs(AsyncResponseStream* response);

// --- 15_About.cpp ---
extern bool OTAInProgress;
extern int  OTAPercentComplete;
void SetupOTA();
void handleAbout(AsyncWebServerRequest *request);
void handleSetASP(AsyncWebServerRequest *request);
void handleSetReboot(AsyncWebServerRequest *request);
void ClearAboutIP();
void SaveAboutIP(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns);


