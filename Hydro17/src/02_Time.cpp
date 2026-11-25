
// =============================================================================
// 02_Time.cpp — MaintainTimeCheck, Software Timers & Date/Time Utilities
// =============================================================================
// Implements the 10 ms MaintainTimeCheck, all software timer management functions,
// NTP time initialisation, and the date/time string utilities shared across
// the irrigation and pool scheduling modules.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// MaintainMidnightReboot  (called every 10 ms from MaintainTimeCheck)
// Nightly scheduled reboot at Params->MidnightRebootTime ("HH:MM", SETTINGS,
// default 00:05):
//   Step 1 — any minute other than the reboot time arms the RAM flag
//            MidnightRebootFlag (only once the clock is set and the unit has
//            been up for 2 minutes).
//   Step 2 — at the reboot time, if the flag is true, the device reboots.
// The flag is RAM only, so after the reboot it is false again and the unit
// cannot reboot twice in the same minute.
// Disabled by Params->MidnightRebootEnabled (SETTINGS tick box).
// ─────────────────────────────────────────────────────────────────────────────
void MaintainMidnightReboot() {
	if (!Params->MidnightRebootEnabled) { MidnightRebootFlag = false; return; }
	if (time(nullptr) < 1600000000) return;   // clock not set yet
	String t = GetCurrentTime();   // "HH:MM"
	if (t != Params->MidnightRebootTime) {
		if (millis() > 120000UL) MidnightRebootFlag = true;
		return;
	}
	if (MidnightRebootFlag) {
		SaveGenReport("REBOOT - Scheduled midnight reboot");
		delay(100);
		ESP.restart();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// MaintainTimeCheck  (called every 10 ms from loop(), main.cpp)
// Resets the watchdog, blinks the LED heartbeat, expires software timers,
// debounces all digital inputs, runs the gateway keepalive, and every
// TIM_FREE_MEMEORY (15 s) runs two software recovery watchdogs:
//   Heap monitor  — reboot if free heap drops below MIN_FREE_HEAP (10 KB)
//   WiFi watchdog — reconnect on drop; reboot after WIFI_LOST_REBOOT_SEC
// It also reboots when LOW_MEM_REBOOT_COUNT web pages in a row were refused
// with "Low memory — retrying" (counted in beginHtmlStream, 04_WebPages.h).
// A stall anywhere in loop() stops watchdogCount being reset, so ISRwatchdog
// (01_Main.cpp) now covers every part of the firmware.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainTimeCheck() {
	static uint32_t WifiDisconnectSince = 0;   // millis() when WiFi first dropped

	watchdogCount = 0;        // Reset watchdog — proves loop() is alive
	LEDToggle();              // LED heartbeat + WiFi credential reset check (01_Main.cpp)
	MaintainTimers();         // Expire elapsed software timers            (02_Time.cpp)
	PinDebounce();            // Debounce all digital inputs                (05_ThisSystem.cpp)
	MaintainMidnightReboot(); // Scheduled nightly reboot (SETTINGS time)  (02_Time.cpp)
	MaintainGatewayWatch();   // KEEPALIVE ping + reboot if target silent 5 min (01_Main.cpp) — do NOT remove
	MaintainWifiNets();       // Apply WiFi network ADD / DELETE from the /wifi page (05_WiFi.cpp)
	MaintainRainGauge();      // RAIN_OK_DO_IRRI from the rain gauge, polled hourly (17_RainGauge.cpp)
	MaintainFwUpdate();       // ABOUT firmware CHECK / INSTALL (blocking, by hand) + restart after upload (22_Update.cpp)

	uint32_t now = millis();

	// ── Low-memory recovery — web pages refused LOW_MEM_REBOOT_COUNT times ───
	// Counted by beginHtmlStream() (04_WebPages.h); rebooted here in loop().
	if (!OTAInProgress && LowMemFailCount >= LOW_MEM_REBOOT_COUNT) {
		SaveGenReport("REBOOT - Low memory: " + String(LowMemFailCount) + " pages refused (free=" +
		              String(LowMemLastFree) + " maxBlock=" + String(LowMemLastMaxBlk) + " B)");
		delay(100);
		ESP.restart();
	}

	// ── Heap + WiFi check every TIM_FREE_MEMEORY (15 s) ──────────────────────
	// Skipped during OTA — flash writes can briefly starve the network stack;
	// heap and WiFi status checks are not meaningful while the OTA TCP stream
	// is holding the connection open.
	if (!OTAInProgress && TimerExpired(TIM_FREE_MEMEORY)) {
		activateTimer(TIM_FREE_MEMEORY);

		// Heap monitor
		uint32_t freeHeap = ESP.getFreeHeap();
		// SaveGenReport("Heap: " + String(freeHeap) + " B");
		if (freeHeap < MIN_FREE_HEAP) {
			SaveGenReport("REBOOT - Low heap: " + String(freeHeap) + " B");
			delay(100);
			ESP.restart();
		}

		// WiFi watchdog
		if (WiFi.status() != WL_CONNECTED) {
			if (WifiDisconnectSince == 0) {
				WifiDisconnectSince = now;
				SaveGenReport("WiFi lost - reconnecting");
				WiFi.reconnect();
			} else if ((now - WifiDisconnectSince) >= (uint32_t)(WIFI_LOST_REBOOT_SEC * 1000)) {
				SaveGenReport("REBOOT - WiFi lost " + String(WIFI_LOST_REBOOT_SEC) + " s");
				delay(100);
				ESP.restart();
			}
		} else {
			if (WifiDisconnectSince != 0) {
				SaveGenReport("WiFi reconnected");
				WifiDisconnectSince = 0;
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupTime
// Configures the ESP32 SNTP client with the South Africa (SAST, UTC+2) timezone
// and the za.pool.ntp.org server, then performs an initial CompileDateTime()
// call so DateTime is valid before the first web page request.
// Called once from setup() after a successful WiFi connection.
// ─────────────────────────────────────────────────────────────────────────────
void SetupTime(){

	// Configure SNTP with South Africa timezone (UTC+2 = SAST)
	configTzTime("SAST-2", ntpServer);

	// Wait (max NTP_SYNC_WAIT_MS) for the first NTP reply so log entries made
	// during boot carry the real date instead of 1970-01-01. getLocalTime()
	// returns once the clock is past 2016, or false on timeout — boot then
	// continues and the clock corrects itself when NTP answers later.
	struct tm ti;
	if (!getLocalTime(&ti, NTP_SYNC_WAIT_MS)) {
		DEBUG_PRINTLN("SetupTime: NTP not synced yet — continuing");
	}

	CompileDateTime();
	DEBUG_PRINTLN(DateTime);
}
 
// ─────────────────────────────────────────────────────────────────────────────
// MaintainTimers  (called every 10 ms from MaintainTimeCheck)
// Iterates over all Timer[] entries and clears Status to false for any timer
// whose ExpireTime has elapsed.  Also auto-restores NormalOperation once
// TIM_ENABLE_NORMAL_OPERATION fires while the controller is disabled.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainTimers(){ // From MaintainTimeCheck
	unsigned long Misec = millis();
	for (int i=0; i < TIM_TOTAL; i++){ 
		if ( Timer[i].Status ){
			if ( Misec > Timer[i].ExpireTime ){
				Timer[i].Status = false;
			}
		} 
	}
 	
	if (!NormalOperation){
		if(TimerExpired(TIM_ENABLE_NORMAL_OPERATION)) { 
			activateTimer(TIM_ENABLE_NORMAL_OPERATION);  
			SwitchNormalOperation();
		}
	}		
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateWebPageTime  (called from loop() via MaintainWebService)
// Once TIM_UPDATE_SYSTEM_TIME (1 s) fires, rebuilds the DateTime string and
// pushes it to connected browsers as element "H02" via the WebSocket.
// Also calls UpdateButtons() to refresh any schedule-dependent button states.
// ─────────────────────────────────────────────────────────────────────────────
void UpdateWebPageTime(){ // From MaintainWebService
	if(TimerExpired(TIM_UPDATE_SYSTEM_TIME)){
		activateTimer(TIM_UPDATE_SYSTEM_TIME); 
		CompileDateTime();
		webParamUpdate(DateTime,"H02","");	
		UpdateButtons();
	}	
}

// ─────────────────────────────────────────────────────────────────────────────
// CompileDateTime
// Reads the current local time from the ESP32 RTC (synced via NTP) and builds
// the DateTime display string in the format "DD Mon YYYY HH:MM:SS Day".
// On day change (MonthDay != today) it triggers midnight tasks:
//   DetermineWeekday(), MaintainSuspention(), ReduceFiles(), MaintainDueDatesValues()
// MaintainSuspention() is called on BOTH the first (bootup) and subsequent
// (midnight) firings, but with isMidnight=false on bootup so rule-b (Once clear)
// is suppressed.  isMidnight=true on every genuine midnight rollover.
// Also updates InternetSec and InternetMin used by the scheduling logic.
// ─────────────────────────────────────────────────────────────────────────────
void CompileDateTime(){
	static bool firstCall = true;  // true only on the very first day-change (bootup)

	time_t now = time(nullptr);
	tm* timeInfo = localtime(&now);
	DateTime = String((timeInfo->tm_mday < 10 ? "0" + String(timeInfo->tm_mday) : String(timeInfo->tm_mday))) + " "
			 + String(YR_MONTH[timeInfo->tm_mon]) + " "
			 + String(timeInfo->tm_year + 1900) + " "
			 + String((timeInfo->tm_hour < 10 ? "0" + String(timeInfo->tm_hour) : String(timeInfo->tm_hour))) + ":"
			 + String((timeInfo->tm_min < 10 ? "0" + String(timeInfo->tm_min) : String(timeInfo->tm_min))) + ":"
			 + String((timeInfo->tm_sec < 10 ? "0" + String(timeInfo->tm_sec) : String(timeInfo->tm_sec))) + " "
			 + String(WEEK_DAY[timeInfo->tm_wday]);

	//Serial.print("CompileDateTime "); Serial.println(DateTime);

	InternetSec = timeInfo->tm_sec;
	InternetMin = timeInfo->tm_min;

	// Every day at Midnight & Startup
	if(MonthDay != timeInfo->tm_mday ){
		MonthDay = timeInfo->tm_mday;
		DetermineWeekday();
		MaintainSuspention(!firstCall);  // false on 1st call (reboot), true from 2nd (midnight)
		firstCall = false;
		ReduceFiles();
		PoolOverride = POOL_OVR_NONE;
		LightManual  = false;
		MaintainDueDatesValues();
	}
}
	
// ─────────────────────────────────────────────────────────────────────────────
// ReduceFiles  (called once at midnight from CompileDateTime)
// Trims all three log files to the maximum entry counts configured in Params.
// ─────────────────────────────────────────────────────────────────────────────
void ReduceFiles(){
	reduce_file("GeneralLog.txt", Params->LogGeneralEvents);
	reduce_file("IrrigationLog.txt", Params->LogIrrigationEvents); 
	reduce_file("TankFillingLog.txt", Params->LogTankFillingEvents); 
}	
	
// ─────────────────────────────────────────────────────────────────────────────
// reduce_file
// Trims a SPIFFS text file to at most Entries lines by copying the most recent
// lines to a temp file then renaming it over the original.  If the file has
// fewer than Entries lines no action is taken.
// ─────────────────────────────────────────────────────────────────────────────
void reduce_file(String filename, int Entries)
{
	int Records = 0; bool Flag = true;
	
	f = SPIFFS.open("/"+filename, "r");
	if (!f) 
	{
		// not Found
	} 
	else 			
	{
		while(f.available())  
		{
			Records++;
			line = f.readStringUntil('\n');	 	
		} 
		f.close();
		
		// Reduce File
		if (Records > Entries){
			int RecDel = Records - Entries;
			Records = 0;
			f = SPIFFS.open("/"+filename, "r");
			if (!f) 
			{
				// not Found
			} 
			else 			
			{	
				ftemp = SPIFFS.open("/temp.txt", "w");
				if (!ftemp)  
				{ 
					// not Found 
				} 
				else 
				{ 
					while(f.available()) {
						line = f.readStringUntil('\n');						
						Records++;		
						if(Records > RecDel){	
							line.trim(); // Removes leading and trailing whitespace, including \r and \n
							ftemp.println(line);        
						}
					} 	  
					f.close();
					ftemp.close();  					
					SPIFFS.remove("/"+filename); 
					SPIFFS.rename("/temp.txt", "/"+filename);					
				}					
			}				
		}
	}
}

// ─── Software Timer Helper Functions ─────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// activateTimer
// Starts the timer at Timer[index] by computing the expiry timestamp from the
// current millis() plus the configured Units × TimerUnits delay, and setting
// Status = true to mark it as counting down.
// ─────────────────────────────────────────────────────────────────────────────
void activateTimer(int index){
//  TIME_TO_EXPIRE            NOW	   + (   TIMER_UNITS (1-???)    X    MS/SEC/MIN   
	Timer[index].ExpireTime = millis() + ( long(Timer[index].Units) *  long(Timer[index].TimerUnits) );
	Timer[index].Status = true;
} 

void deactivateTimer(int index){
	Timer[index].Status = false;  
}

bool TimerExpired(int index){
	return !Timer[index].Status;
}

void SetValueAndAtivate(String Units, int TimerUnits, int index ){
	Timer[index].Units = Units.toInt();	
	Timer[index].TimerUnits = TimerUnits;	
	activateTimer(index);
} 

void SetTimerValue(int Units, int index ){
	Timer[index].Units = Units;	
} 

int RemainingSec(int index){
	unsigned long Misec = millis();
	if ( Timer[index].Status ){
		if ( Misec > Timer[index].ExpireTime ) 
		{
			return 0;
		}
		else
		{
			return ( Timer[index].ExpireTime - Misec ) / SEC;
		}
	}
	else{
		return 0;
	} 
} 

int RemainingMin(int index){
	unsigned long Misec = millis();
	if ( Timer[index].Status ){
		if ( Misec > Timer[index].ExpireTime ) 
		{
			return 0;
		}
		else
		{
			return ( Timer[index].ExpireTime - Misec ) / MIN;
		}
	}
	else{
		return 0;
	} 
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateTimerTimes
// Syncs configurable timer durations from Params and TankWaterLevel into the
// Timer[] array.  Called from InitOther() at startup and whenever settings change.
// ─────────────────────────────────────────────────────────────────────────────
void UpdateTimerTimes(){
	SetTimerValue(TankWaterLevel->FilterPumpDelay, TIM_TANK_ON_DELAY );
	SetTimerValue(Params->ResetTimeOnContDisabled, TIM_ENABLE_NORMAL_OPERATION );
}

// ─── Date & Time String Utility Functions ────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// IncrementDateWithOneDay
// Adds one calendar day to a date string in "YYYY-MM-DD" format, handling
// month-end roll-over and December → January year increment.
// Returns the new date as "YYYY-MM-DD".
// ─────────────────────────────────────────────────────────────────────────────
String IncrementDateWithOneDay(String Date){
	int Year = Date.substring(0,4).toInt(); 
	int Month = Date.substring(5,7).toInt();
	int Day = Date.substring(8,10).toInt();	
	if( Day == MonthDaysValue(Year, Month) ) {
 		if(Month == 12) {
			Year++;
			Month = 1;
			Day = 1;			
		} else {
			Month++;
			Day = 1;
		}	
	}
	else {
		Day++;
	}
	return IntToStringYearMonthDay(Year, Month, Day);		
}

String AddTimeToCurrentDateTime(String Date, String Time)
{
	String ToReturn = Date.substring(0,10);
	int DateHour = Date.substring(11,13).toInt();
	int DateMin = Date.substring(14,16).toInt();	
	int PeriodHour = Time.substring(0,2).toInt();
	int PeriodMin = Time.substring(3,5).toInt();		

	int NewMin = DateMin + PeriodMin;
	if(NewMin > 59)
	{
		NewMin = NewMin - 60;		
		PeriodHour = PeriodHour + 1;		
	}
 
	int NewHour = DateHour + PeriodHour;
	if(NewHour > 23)
	{
		NewHour = NewHour - 24;
		ToReturn = IncrementDateWithOneDay(ToReturn);
	}
	
	ToReturn += " "; ToReturn += IntToStringHourMinute(NewHour, NewMin);	
	return ToReturn;
}

int MonthDaysValue(int Year, int Month)
{
	if( Month == 2 ) // Check for Leap year
	{
		if ( (Year % 4) == 0){ 
			return 29;	// Leap year 
		} else { 
			return 28;	// Not Leap year
		}		
	} else {
		return MonthLen[Month - 1];		
	}	
}

void DetermineWeekday() // Since - Ref 2023/11/07 = Wednesday : Weekday = 3	
{		
	time_t now = time(nullptr);
	tm* timeInfo = localtime(&now);
	if ( (timeInfo->tm_year + 1900) >= 2021 ) 
	{
		WeekDay = 3; //  1 = Sunday .... 7 Saturday
		byte localday = 7;
		byte localMonth = 11;
		int localyear = 2023;
		while (	localday != timeInfo->tm_mday || localMonth != (timeInfo->tm_mon + 1) || localyear != (timeInfo->tm_year + 1900) )			
		{	
			localday++; 
			WeekDay++; 
			if ( WeekDay == 8 )  WeekDay = 1;
			if ( localday > MonthDaysValue(localyear, localMonth))
			{			
				localday = 1;
				localMonth++;
				if ( localMonth == 13 )
				{
					localMonth = 1;
					localyear++;
				}
			}	
		}
	}
	DEBUG_PRINT("WeekDay = "); DEBUG_PRINTLN(WeekDay);
}

String GetCurrentDateAndTime() {
	time_t now = time(nullptr);
	tm* timeInfo = localtime(&now);
	String CurrentDateTime = IntToStringYearMonthDay (timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
	CurrentDateTime += " "; CurrentDateTime += IntToStringHourMinute(timeInfo->tm_hour, timeInfo->tm_min);
	return CurrentDateTime;	
}	
 
String GetCurrentDate(int DaysAdded){ 
	int days = DaysAdded;
	time_t now = time(nullptr);
	tm* timeInfo = localtime(&now);
	String CurrentDate = IntToStringYearMonthDay (timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
	while (days > 0)
	{
		CurrentDate = IncrementDateWithOneDay(CurrentDate);
		days--;	
	}
	return CurrentDate;	
}

String GetCurrentTime(){
	time_t now = time(nullptr);
	tm* timeInfo = localtime(&now);	
	return IntToStringHourMinute(timeInfo->tm_hour, timeInfo->tm_min);
}

String IntToStringYearMonthDay (int Year, int Month, int Day) {
	return (String(Year) + "-"                                                 // Year  
			+ String((Month < 10 ? "0" + String(Month) : String(Month))) + "-" // Month  
			+ String((Day < 10 ? "0" + String(Day) : String(Day))));           // Day  
} 
  
String IntToStringHourMinute(int Hour, int Minute) {	
	if(Hour == 0) {
		return ( "00:"                                                        // Hour  
			+ String(Minute < 10 ? "0" + String(Minute) : String(Minute)));   // Min  		
	}
	else
	{
		return ( String(Hour < 10 ? "0" + String(Hour) : String(Hour)) + ":"  // Hour  
			+ String(Minute < 10 ? "0" + String(Minute) : String(Minute)));   // Min  
	}
}

String GetHrMinSec(int Sec, bool RetHours){

	int Min = Sec / 60 ;  // Total minutes
	int Second = Sec - (Min * 60);
	int Hour = Min / 60; // Total Hours
	int Minute = Min - (Hour * 60);
	String Addhour = "";
	if (RetHours) Addhour = "00:";

	if(Hour == 0)
	{
		return ( Addhour
	           + String(Minute < 10 ? "0" + String(Minute) : String(Minute)) + ":"   // Min 
			   + String(Second < 10 ? "0" + String(Second) : String(Second)) );      // Sec	
	}
	else 
	{
		return ( String(Hour < 10 ? "0" + String(Hour) : String(Hour)) + ":"      // Hour  
			+ String(Minute < 10 ? "0" + String(Minute) : String(Minute)) + ":"   // Min 
			+ String(Second < 10 ? "0" + String(Second) : String(Second)) );      // Sec
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// GetDifferenceTimeStartBusy
// Calculates the remaining time between TimeStart (total period) and TimeBusy
// (elapsed so far), both in "HH:MM:SS" format.
// Always returns "HH:MM:SS" so the display format is consistent regardless of
// whether hours are zero — e.g., "00:45:00" not "45:00".
// ─────────────────────────────────────────────────────────────────────────────
String GetDifferenceTimeStartBusy(String TimeStart, String TimeBusy) {  //  HH:MM:SS  HH:MM:SS

	int Start = (TimeStart.substring(0,2).toInt() * 3600) + (TimeStart.substring(3,5).toInt() * 60) + TimeStart.substring(6,8).toInt();	
	int Busy = (TimeBusy.substring(0,2).toInt() * 3600) + (TimeBusy.substring(3,5).toInt() * 60) + TimeBusy.substring(6,8).toInt();		

	int ResultSeconds = Start - Busy;
	if (ResultSeconds > 3599) // Greater than 1 Hour
	{
		int ResultHR = ResultSeconds / 3600;		
		int ResultMIN = ( ResultSeconds - ( ResultHR * 3600 ) ) / 60;		
		int ResultSEC = ( ResultSeconds - ( ResultHR * 3600 ) -  ( ResultMIN * 60 ) );		
		
		return ( String(ResultHR < 10 ? "0" + String(ResultHR) : String(ResultHR)) + ":"       // Hour
			   + String( ResultMIN < 10 ? "0" + String( ResultMIN) : String( ResultMIN)) + ":" // Min	
			   + String( ResultSEC < 10 ? "0" + String( ResultSEC) : String( ResultSEC)) );    // Sec			
	}
	else if(ResultSeconds > 59) // Greater than 1 Minute, less than 1 Hour
	{
		int ResultMIN = ResultSeconds / 60;
		int ResultSEC = ResultSeconds - ( ResultMIN * 60 );

		return ( "00:"                                                                                   // Hour
			   + String( ResultMIN < 10 ? "0" + String( ResultMIN) : String( ResultMIN)) + ":"          // Min
			   + String( ResultSEC < 10 ? "0" + String( ResultSEC) : String( ResultSEC)) );             // Sec
	}
	else  // Less than 1 Minute
	{
		return ( "00:00:"                                                                                // Hour:Min
			   + String( ResultSeconds < 10 ? "0" + String(ResultSeconds) : String(ResultSeconds)) );  // Sec
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// GetDifferenceBetweenTwoDates
// Counts the number of calendar days between StartDate and EndDate (inclusive
// of neither boundary).  Both dates must be in "YYYY-MM-DD" format.
// Returns 0 if StartDate > EndDate (i.e. the start is already past the end).
// ─────────────────────────────────────────────────────────────────────────────
int GetDifferenceBetweenTwoDates(String StartDate, String EndDate){
	
	int Year = StartDate.substring(0,4).toInt(); 
	int Month = StartDate.substring(5,7).toInt();
	int Day = StartDate.substring(8,10).toInt();	

	int EndYear = EndDate.substring(0,4).toInt(); 
	int EndMonth = EndDate.substring(5,7).toInt();
	int EndDay = EndDate.substring(8,10).toInt();

	int TotalDays = 0;
	
	if(StartDate <= EndDate)
	{
		while( (Year != EndYear) || (Month != EndMonth) || (Day != EndDay) )
		{
			if( Day == MonthDaysValue(Year, Month) ) {
				if(Month == 12) {
					Year++;
					Month = 1;
					Day = 1;	
					TotalDays++;				
				} else {
					Month++;
					Day = 1;
					TotalDays++;					
				}	
			}
			else {
				Day++;
				TotalDays++;				
			}
		}
	}

	return TotalDays;
}
 