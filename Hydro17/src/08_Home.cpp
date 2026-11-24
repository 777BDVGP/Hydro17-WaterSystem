// =============================================================================
// 08_Home.cpp — Home Page Handler, Status Maintenance & Due Date Display
// =============================================================================
// Implements the root page (/) handler, the PageVar map for the home page,
// and the maintenance functions that push live system status strings to
// connected browsers via WebSocket every time a value changes.
// Due date rows (pool backwash, tank backwash, filter replacements) are
// recalculated once per day by MaintainDueDatesValues().
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handleRoot  (GET /)
// Serves the Home page HTML.  Allocates HomeList on first call (MainPageObject
// flag prevents double-allocation); subsequent calls reuse the existing list.
// ─────────────────────────────────────────────────────────────────────────────
void handleRoot(AsyncWebServerRequest *request){
	//SaveGenReport("handleRoot f=" + String(ESP.getFreeHeap()) + " m=" + String(ESP.getMaxAllocHeap()));

	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	appendMainPageSel(PS_HOME, response);
	response->print(F("<style>table td,table th{padding:1px 6px}table tr:first-child th:last-child{text-align:right!important}</style>"));
	response->print(webScriptTag);
	WT_HomePage(response);
	response->print(webHTMLEnd);
	request->send(response);
}

// Page Main Variables

// HOME IRRIGATION TANK POOL PLUGS REPORTS SETTINGS FILES

void appendMainPageSel(int Index, AsyncResponseStream* r){
	r->print(F("<div class='nav'>"));
	r->print((Index == PS_HOME)       ? "<a class='act'>HOME</a>"        : "<a href='/'>HOME</a>");
	r->print((Index == PS_IRRIGATION) ? "<a class='act'>IRRIGATION</a>"  : "<a href='/irrigation'>IRRIGATION</a>");
	r->print((Index == PS_TANK)       ? "<a class='act'>TANK</a>"        : "<a href='/tank'>TANK</a>");
	r->print((Index == PS_POOL)       ? "<a class='act'>POOL FILTER</a>" : "<a href='/pool'>POOL FILTER</a>");
	r->print((Index == PS_POOL_LIGHT) ? "<a class='act'>POOL LIGHT</a>"  : "<a href='/poollight'>POOL LIGHT</a>");
	r->print((Index == PS_OTHER)      ? "<a class='act'>OTHER</a>"       : "<a href='/other'>OTHER</a>");
	r->print(F("</div>"));
}
 
void WT_HomePage(AsyncResponseStream* response){

// 	Create List of Setting Page Parameters
	resetPageBuf();
 	WT_StartPage();
  	WT_StartTable("DESCRIPTIONS", "VALUES", "");

	for (int i=0; i < PAGE_VAR_HOME; i++)
	{
		WT_DisplayRow(&HomeList[i]);
	}

	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}
 
void MapHomePage(){
	MainPageObject = true; 
//  In this routine a list, HomeList, is setup to connect the Webpage and the System Variables
	HomeList = new PageVar[PAGE_VAR_HOME];
	String DefaultStyle = F("color: black; font-size:14px");  // Normal 14
	{ 
		for (int i=0; i < PAGE_VAR_HOME; i++){	
			switch(i)    
			{ 	
				case HOME_CONTROLLERST: 	 
					HomeList[i].Var = new PageElement(&StrControllerStatus, F("Controller Status:"), DefaultStyle, &StrControllerStatusAtt, i+ID_HOME);  	
					break;
				case HOME_BORHOLESTATU: 
					HomeList[i].Var = new PageElement(&StrBoreholeStatus, F("Borehole Status:"), DefaultStyle, &StrBoreholeStatusAtt, i+ID_HOME);  		
					break;
				case HOME_TANKSUPPIRRI:
					HomeList[i].Var = new PageElement(&StrTankSupplyIrri, F("Tank Irrigation Valve:"), DefaultStyle, &StrTankSupplyIrriAtt, i+ID_HOME);  		
					break; 						
				case HOME_TANKFILLINGS:
					HomeList[i].Var = new PageElement(&StrTankFilling, F("Tank Filling Status:"), DefaultStyle, &StrTankFillingAtt, i+ID_HOME);  	
					break;		
				case HOME_TANKFILTER:
					HomeList[i].Var = new PageElement(&StrTankFilter, F("Tank Filter Status:"), DefaultStyle, &StrTankFilterAtt, i+ID_HOME);  	
					break;						
				case HOME_TANKFLOATLEV:
					HomeList[i].Var = new PageElement(&StrTankFloatSensor, F("Tank Float Status:"), DefaultStyle, &StrTankFloatSensorAtt, i+ID_HOME);  	
					break;					
				case HOME_TANKLEVEL:
					HomeList[i].Var = new PageElement(&StrTankLevel, F("Tank Level:"), DefaultStyle, &StrTankLevelAtt, i+ID_HOME);
					break;
				case HOME_FILTER_PRESSURE:
					HomeList[i].Var = new PageElement(&StrFilterPressure, F("Filter Pressure:"), DefaultStyle, &StrFilterPressureAtt, i+ID_HOME);
					break;
				case HOME_IRRIGATION:
					HomeList[i].Var = new PageElement(&StrIrrigate, F("Irrigation Activity:"), DefaultStyle, &StrIrrigateAtt, i+ID_HOME);
					break;						
				case HOME_POOLFILTER:
					HomeList[i].Var = new PageElement(&StrPoolFilter, F("Pool Filter Status:"), DefaultStyle, &StrPoolFilterAtt, i+ID_HOME);  
					break;	
				case HOME_POOLFILTERMAN:
					HomeList[i].Var = new PageElement(&StrPoolFilterMan, F("Pool Filter Operation:"), DefaultStyle, &StrPoolFilterManAtt, i+ID_HOME);
					break;						
				case HOME_POOLLIGHT:
					HomeList[i].Var = new PageElement(&StrPoolLight, F("Pool Light Status:"), DefaultStyle, &StrPoolLightAtt, i+ID_HOME);
					break;
				case HOME_POOLLIGHTMAN:
					HomeList[i].Var = new PageElement(&StrPoolLightMan, F("Pool Light Operation:"), DefaultStyle, &StrPoolLightManAtt, i+ID_HOME);
					break;
//				case HOME_PLUGS:
//					HomeList[i].Var = new PageElement(&StrPlugs, F("Plugs Status:"), DefaultStyle, &StrPlugsAtt, i+ID_HOME);
//					break;
				case HOME_POOL_BACKWASH:
				    HomeList[i].Var = new PageElement(&StrNextPoolBack, F("Backwash Pool Filter :"), DefaultStyle, &StrNextPoolBackAtt, i+ID_HOME);  	
					break;				
				case HOME_TANK_BACKWASH:
                    HomeList[i].Var = new PageElement(&StrNextTankBack, F("Backwash Tank Filter:"), DefaultStyle, &StrNextTankBackAtt, i+ID_HOME);  	
					break;					
				case HOME_CLEAN_BH_FILTER:
                    HomeList[i].Var = new PageElement(&StrNextBHFilter, F("Clean Hydrocyclone:"), DefaultStyle, &StrNextBHFilterAtt, i+ID_HOME);  				
					break;	
				case HOME_REP_BLUE_FILTER:
                    HomeList[i].Var = new PageElement(&StrNextRepBlueFil, F("Replace Blue Filter:"), DefaultStyle, &StrNextRepBlueFilAtt, i+ID_HOME);  
					break;						
				case HOME_UV_TUBE_REPLACE:
                    HomeList[i].Var = new PageElement(&StrUVTubeReplace, F("Replace UV-Tube:"), DefaultStyle, &StrUVTubeReplaceAtt, i+ID_HOME);
					break;
				case HOME_CHLORINE_MAINT:
                    HomeList[i].Var = new PageElement(&StrChlorineMaint, F("Add Pool Chlorine:"), DefaultStyle, &StrChlorineMainAtt, i+ID_HOME);
					break;
			}
		}
	}
}
	
void webRebootButton(AsyncResponseStream* r)
{
	r->print(F("<div><br></div><div><b><a>"
	           "<b><a><input type=\"button\" id=\"REBOOT\" onclick=\"ButtonReboot()\" "
	           "style=\"font-size:14px;background-color:#dc2626;color:#fff;"
	           "border:none;padding:6px 14px;border-radius:4px;cursor:pointer\" "
	           "value=\"REBOOT\"</a></b></div>"));
}

void MaintainHomePageValues(){	
	for (int i=0; i < PAGE_VAR_HOME; i++)
	{ 
		if(HomeList[i].Var->GetName() != HomeList[i].Var->GetNameLast())
		{
			webParamUpdate(HomeList[i].Var->GetName(),HomeList[i].Var->GetID("V"),HomeList[i].Var->GetVarStyle());	
			HomeList[i].Var->UpdateLast();							
		}	
	}	
}

void ForceHomeIrrigatePush(){
	HomeList[HOME_IRRIGATION].Var->ClearLast();
}

void MaintainHomePage(){
	
	if(NormalOperation){
		StrControllerStatus    = HOME_CONTROL_ACTIVE;
		StrControllerStatusAtt = ATT_GREEN;
	} else {
		StrControllerStatus    = HOME_CONTROL_NACTIVE;
		StrControllerStatusAtt = ATT_RED;
	}

	if(!digitalRead(OutPort[O_BOREHOLE].Port)){
		StrBoreholeStatus      = HOME_BOREHO_STAT_NOTB;
		StrBoreholeStatusAtt   = ATT_GREEN;
	} else {
		StrBoreholeStatus      = HOME_BOREHO_STAT_BUSY;
		StrBoreholeStatusAtt   = ATT_ORANGE;
	}

	if(!digitalRead(OutPort[O_TANK_SUPPLY_SOLENOID].Port)){
		StrTankSupplyIrri      = HOME_TANKSUPPLY_NO;
		StrTankSupplyIrriAtt   = ATT_GREEN;
	} else {
		StrTankSupplyIrri      = HOME_TANKSUPPLY_YES;
		StrTankSupplyIrriAtt   = ATT_ORANGE;
	}

	if(!digitalRead(OutPort[O_TANK_FILL_SOLENOID].Port)){
		StrTankFilling         = HOME_TANKFILL_NO_BUSY;
		StrTankFillingAtt      = ATT_GREEN;
	} else {
		int TankFillDur = ( millis() / 1000 ) - StartValveTime;		
		StrTankFilling = HOME_TANKFILL_BUSY; 
		StrTankFilling += " "; StrTankFilling += GetHrMinSec(TankFillDur, true);
		StrTankFillingAtt      = ATT_ORANGE;		
	}


	if(!digitalRead(OutPort[O_TANK_FILTER_PUMP_RELAY].Port)){
		StrTankFilter    = HOME_TANKFILTER_OFF;
		StrTankFilterAtt = ATT_GREEN;
	} else if(digitalRead(OutPort[O_BOREHOLE].Port)) {
		StrTankFilter    = HOME_TANKFILON_PUMPON;
		StrTankFilterAtt = ATT_ORANGE;
	} else {
		StrTankFilter    = HOME_TANKFILTER_ON;
		StrTankFilter   += " "; StrTankFilter += GetHrMinSec(RemainingSec(TIM_TANK_ON_DELAY), true);
		StrTankFilterAtt = ATT_YELLOW;
	}
	
	if(!NormalOperation){  
		StrTankFloatSensor     = HOME_TANKFLOAT_DIS;
		StrTankFloatSensorAtt  = ATT_RED;			
	}
	else if(digitalRead(OutPort[O_TANK_HIGH_LED].Port)){
		StrTankFloatSensor     = HOME_TANKFLOAT_HIGH;
		StrTankFloatSensorAtt  = ATT_GREEN;
	}
	else { 
		if(AllowFillingAtTime)
		{
			StrTankFloatSensor  = HOME_TANKFLOAT_LOW;			
		}
		else
		{
			StrTankFloatSensor  = HOME_TANKFLOAT_LOW_TIM;			
		}
		StrTankFloatSensorAtt  = ATT_ORANGE;	
	}
	
	StrTankLevel = String(TankWaterLevel->getValue()) + "%";
	if(TankWaterLevel->getValue() < TankWaterLevel->StartFillLevel)
	{
		if(!AllowFillingAtTime)
		{
			StrTankLevel = String(TankWaterLevel->getValue()) + "% - " + "Waiting Time";
		}
		StrTankLevelAtt = ATT_RED;
	}
	else
	{
		StrTankLevelAtt = ATT_GREEN;
	}

	StrFilterPressure = FilterSensor->StrFilterKPa;
	if(FilterSensor->FilterPressureExceeded) {
		StrFilterPressureAtt = ATT_RED;
	} else if(FilterSensor->getValue() > FilterSensor->BackwashWarning * 0.8) {
		StrFilterPressureAtt = ATT_ORANGE;
	} else {
		StrFilterPressureAtt = ATT_GREEN;
	}

    if(SUSPEND_IRRI)
	{
		StrIrrigate    = HOME_IRRIGATION_SUSP;
		StrIrrigateAtt = ATT_RED;
	}
	else
	{
		StrIrrigate    = HOME_IRRIGATION_OFF;
		StrIrrigateAtt = ATT_GREEN;
		for ( int x = 0; x < IRRIGATE_SELECTIONS; x++ ){
			if(SheduleObject->IrrigateShedule[x].ShedStat == SHED_STATUS_ACTIVE){	
 				int IrriDur = ( millis() / 1000 ) - StartValveTime;				
				StrIrrigate = IrriObject->Selection[x].SelectDescription; StrIrrigate += " "; 
				StrIrrigate += GetDifferenceTimeStartBusy(IrriObject->Selection[x].GetPeriodToIrrigateSeconds(), GetHrMinSec(IrriDur,true));	
				StrIrrigateAtt  = ATT_ORANGE;			
				x = IRRIGATE_SELECTIONS;
			}
		}
	}

	if(!digitalRead(OutPort[O_POOL_PUMP_RELAY].Port)){
		StrPoolFilter     = HOME_POOLFILTER_OFF;
		StrPoolFilterAtt  = ATT_GREEN;
	} else {
		StrPoolFilter     = HOME_POOLFILTER_ON;
		StrPoolFilterAtt  = ATT_YELLOW;
	}

	if(PoolOverride == POOL_OVR_NONE){
		StrPoolFilterMan    = HOME_POOLFILTERMAN_OFF;
		StrPoolFilterManAtt = ATT_GREEN;
	} else {
		StrPoolFilterMan    = HOME_POOLFILTERMAN_ON;
		StrPoolFilterManAtt = ATT_RED;
	}

	if(!digitalRead(OutPort[O_POOL_LIGHT_RELAY].Port)){
		StrPoolLight     = HOME_POOLLIGHT_OFF;
		StrPoolLightAtt  = ATT_GREEN;
	} else {
		StrPoolLight     = HOME_POOLLIGHT_ON;
		StrPoolLightAtt  = ATT_YELLOW;
	}

	if(!LightManual){
		StrPoolLightMan    = HOME_POOLLIGHTMAN_OFF;
		StrPoolLightManAtt = ATT_GREEN;
	} else {
		StrPoolLightMan    = HOME_POOLLIGHTMAN_ON;
		StrPoolLightManAtt = ATT_RED;
	}

//	if(!digitalRead(OutPort[O_PLUGS_SWITCH_RELAY].Port)){
//		StrPlugs         = HOME_PLUGS_OFF;
//		StrPlugsAtt      = ATT_TURQU;
//	} else {
//		StrPlugs         = HOME_PLUGS_ON;
//		StrPlugsAtt      = ATT_ORANGE;
//	}
	
	StrNextPoolBack = NtfyData->NextPoolBackwashDate;
	if(GetCurrentDate(0) >= NtfyData->NextPoolBackwashDate)
	{
		StrNextPoolBack    = "Do Backwash";
		StrNextPoolBackAtt = ATT_RED;
	}
	else
	{
		StrNextPoolBack += " - "; StrNextPoolBack += String(DaysToBackwashPool); StrNextPoolBack += " days";
		StrNextPoolBackAtt = ATT_GREEN;
	}

	StrNextTankBack = NtfyData->NextTankBackwashDate;
	if(GetCurrentDate(0) >= NtfyData->NextTankBackwashDate)
	{
		StrNextTankBack    = "Do Backwash";
		StrNextTankBackAtt = ATT_RED;
	}
	else
	{
		StrNextTankBack += " - "; StrNextTankBack += String(DaysToBackwashTank); StrNextTankBack += " days";
		StrNextTankBackAtt = ATT_GREEN;
	}

	StrNextBHFilter = NtfyData->NextBHCleanDate;
	if(GetCurrentDate(0) >= NtfyData->NextBHCleanDate)
	{
		StrNextBHFilter    = "Flushing Hydrocyclone";
		StrNextBHFilterAtt = ATT_RED;
	}
	else
	{
		StrNextBHFilter += " - "; StrNextBHFilter += String(DaysToCleanBHFilter); StrNextBHFilter += " days";
		StrNextBHFilterAtt = ATT_GREEN;
	}

	StrNextRepBlueFil = NtfyData->NextBlueFilterDate;
	if(GetCurrentDate(0) >= NtfyData->NextBlueFilterDate)
	{
		StrNextRepBlueFil    = "Replace Filter";
		StrNextRepBlueFilAtt = ATT_RED;
	}
	else
	{
		StrNextRepBlueFil += " - "; StrNextRepBlueFil += String(DaysToReplaceBlueFilter); StrNextRepBlueFil += " days";
		StrNextRepBlueFilAtt = ATT_GREEN;
	}

	StrUVTubeReplace = NtfyData->NextUVTubeReplaceDate;
	if(GetCurrentDate(0) >= NtfyData->NextUVTubeReplaceDate)
	{
		StrUVTubeReplace    = "Replace UV Tube";
		StrUVTubeReplaceAtt = ATT_RED;
	}
	else
	{
		StrUVTubeReplace += " - "; StrUVTubeReplace += String(DaysToReplaceUVTube); StrUVTubeReplace += " days";
		StrUVTubeReplaceAtt = ATT_GREEN;
	}

	StrChlorineMaint = NtfyData->NextChlorineMaintDate;
	if(GetCurrentDate(0) >= NtfyData->NextChlorineMaintDate)
	{
		StrChlorineMaint    = "Maintain Chlorine";
		StrChlorineMainAtt  = ATT_RED;
	}
	else
	{
		StrChlorineMaint += " - "; StrChlorineMaint += String(DaysToChlorine); StrChlorineMaint += " days";
		StrChlorineMainAtt = ATT_GREEN;
	}

}
 
void MaintainDueDatesValues(){
	// On Bootup and Once a day
	DaysToBackwashPool      = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextPoolBackwashDate);
	DaysToBackwashTank      = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextTankBackwashDate);
	DaysToCleanBHFilter     = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextBHCleanDate);
	DaysToReplaceBlueFilter = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextBlueFilterDate);
	DaysToReplaceUVTube     = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextUVTubeReplaceDate);
	DaysToChlorine          = GetDifferenceBetweenTwoDates(GetCurrentDate(0), NtfyData->NextChlorineMaintDate);
 }