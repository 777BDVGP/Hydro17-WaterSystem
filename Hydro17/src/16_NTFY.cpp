
// =============================================================================
// 16_NTFY.cpp — NTFY.sh Push Notifications, Due-Date Alerts & NTFY Web Page
// =============================================================================
// Implements the NtfyParams class (JSON persistence to /NTFY.json), the
// FreeRTOS NtfyTask that posts HTTP messages to ntfy.sh, MaintainNTFY() for
// daily overdue-date alerts, and the /ntfy web page for configuration.
// =============================================================================

// ─── NtfyParams Class ─────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// CreateNtfyParams
// Allocates the global NtfyParams instance, loads saved values from SPIFFS,
// writes back defaults for any missing keys, then builds the NTFY page-variable
// map.  Called from InitOperationalObjects().
// ─────────────────────────────────────────────────────────────────────────────
void CreateNtfyParams() {
	NtfyData = new NtfyParams;
	NtfyData->ReadData();
	NtfyData->WriteData();
	MapNtfyPage();
}

NtfyParams::NtfyParams()  {}
NtfyParams::~NtfyParams() {}

void NtfyParams::WriteData()
{
	String StrFileName = "/NTFY.json";
	File f = SPIFFS.open(StrFileName, "w");
	JsonDocument doc;

	doc["NtfyCode"]              = NtfyCode;
	doc["NtfyPriority"]          = NtfyPriority;
	doc["NtfyMessageTime"]       = NtfyMessageTime;
	doc["NextPoolBackwashDate"]  = NextPoolBackwashDate;
	doc["NextTankBackwashDate"]  = NextTankBackwashDate;
	doc["NextBHCleanDate"]       = NextBHCleanDate;
	doc["NextBlueFilterDate"]    = NextBlueFilterDate;
	doc["NextUVTubeReplaceDate"] = NextUVTubeReplaceDate;
	doc["NextChlorineMaintDate"] = NextChlorineMaintDate;
	doc["DaysPoolBackwash"]      = DaysPoolBackwash;
	doc["DaysTankBackwash"]      = DaysTankBackwash;
	doc["DaysFlushHydro"]        = DaysFlushHydro;
	doc["DaysBlueFilter"]        = DaysBlueFilter;
	doc["DaysUVTube"]            = DaysUVTube;
	doc["DaysChlorine"]          = DaysChlorine;

	if (f)
	{
		serializeJsonPretty(doc, f);
		f.close();
	}
}

void NtfyParams::ReadData()
{
	String StrFileName = "/NTFY.json";
	File f = SPIFFS.open(StrFileName, "r");
	if (f)
	{
		size_t size = f.size();
		if (size <= 800)
		{
			std::unique_ptr<char[]> buf(new char[size]);
			f.readBytes(buf.get(), size);
			JsonDocument doc;
			auto error = deserializeJson(doc, buf.get());
			if (!error)
			{
				if (doc["NtfyCode"].as<String>()             != "null") { NtfyCode             = doc["NtfyCode"].as<String>(); }
				if (doc["NtfyPriority"].as<String>()         != "null") { NtfyPriority         = doc["NtfyPriority"].as<String>(); }
				if (doc["NtfyMessageTime"].as<String>()      != "null") { NtfyMessageTime      = doc["NtfyMessageTime"].as<String>(); }
				if (doc["NextPoolBackwashDate"].as<String>()  != "null") { NextPoolBackwashDate  = doc["NextPoolBackwashDate"].as<String>(); }
				if (doc["NextTankBackwashDate"].as<String>()  != "null") { NextTankBackwashDate  = doc["NextTankBackwashDate"].as<String>(); }
				if (doc["NextBHCleanDate"].as<String>()       != "null") { NextBHCleanDate       = doc["NextBHCleanDate"].as<String>(); }
				if (doc["NextBlueFilterDate"].as<String>()    != "null") { NextBlueFilterDate    = doc["NextBlueFilterDate"].as<String>(); }
				if (doc["NextUVTubeReplaceDate"].as<String>() != "null") { NextUVTubeReplaceDate = doc["NextUVTubeReplaceDate"].as<String>(); }
				if (doc["NextChlorineMaintDate"].as<String>() != "null") { NextChlorineMaintDate = doc["NextChlorineMaintDate"].as<String>(); }
				if (doc["DaysPoolBackwash"].as<String>()      != "null") { DaysPoolBackwash = doc["DaysPoolBackwash"].as<int>(); }
				if (doc["DaysTankBackwash"].as<String>()      != "null") { DaysTankBackwash = doc["DaysTankBackwash"].as<int>(); }
				if (doc["DaysFlushHydro"].as<String>()        != "null") { DaysFlushHydro   = doc["DaysFlushHydro"].as<int>();   }
				if (doc["DaysBlueFilter"].as<String>()        != "null") { DaysBlueFilter   = doc["DaysBlueFilter"].as<int>();   }
				if (doc["DaysUVTube"].as<String>()            != "null") { DaysUVTube       = doc["DaysUVTube"].as<int>();       }
				if (doc["DaysChlorine"].as<String>()          != "null") { DaysChlorine     = doc["DaysChlorine"].as<int>();     }
			}
			else
			{
				SaveGenReport F(" File Error open File NTFY.json");
			}
		}
		else
		{
			SaveGenReport F(" File NTFY.json size is > 700 ");
		}
		f.close();
	}
}

/* class NtfyParams - END ---------------------------------------------------- */

// ─────────────────────────────────────────────────────────────────────────────
// MapNtfyPage
// Builds the NtfyList PageVar array that connects the NTFY web page form
// elements to the NtfyParams fields via WebSocket.
// Called from CreateNtfyParams() once at boot.
// ─────────────────────────────────────────────────────────────────────────────
void MapNtfyPage() {
	NtfyPageObject = true;
	NtfyList = new PageVar[PAGE_VAR_NTFY];
	for (int i = 0; i < PAGE_VAR_NTFY; i++)
	{
		switch (i)
		{
			case NTFY_DATE_BACKWASH_POOL:
				NtfyList[i].Var = new PageElement(&NtfyData->NextPoolBackwashDate,   F("Next Pool Backwash"),         "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_DATE_BACKWASH_TANK:
				NtfyList[i].Var = new PageElement(&NtfyData->NextTankBackwashDate,   F("Next Tank Backwash"),         "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_DATE_CLEAN_BOREHOLE:
				NtfyList[i].Var = new PageElement(&NtfyData->NextBHCleanDate,        F("Next Flushing Hydrocyclone"), "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_DATE_REPL_BLUE_FILTER:
				NtfyList[i].Var = new PageElement(&NtfyData->NextBlueFilterDate,     F("Next Replace Blue Filter"),  "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_DATE_REPLACE_UV_TUBE:
				NtfyList[i].Var = new PageElement(&NtfyData->NextUVTubeReplaceDate,  F("Next UV Tube Replacement"),  "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_DATE_CHLORINE_MAINT:
				NtfyList[i].Var = new PageElement(&NtfyData->NextChlorineMaintDate,  F("Next Chlorine Maintenance"), "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_CODE:
				NtfyList[i].Var = new PageElement(&NtfyData->NtfyCode,        F("NTFY Topic Name"),   "", &NtfyStyle, i + ID_NTFY);
				break;
			case NTFY_PRIORITY:
				NtfyList[i].Var = new PageElement(&NtfyData->NtfyPriority,    F("Alert Level (on the phone)"), "", &NtfyStyle, i + ID_NTFY);
				NtfyList[i].Var->SetStringToSelect(5);
				NtfyList[i].Var->AddOption("min");
				NtfyList[i].Var->AddOption("low");
				NtfyList[i].Var->AddOption("default");
				NtfyList[i].Var->AddOption("high");
				NtfyList[i].Var->AddOption("urgent");
				break;
			case NTFY_TIME:
				NtfyList[i].Var = new PageElement(&NtfyData->NtfyMessageTime, F("Daily Reminder Time"), "", &NtfyStyle, i + ID_NTFY);
				break;
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// handleApplyDate  (GET /applydate?field=X&days=N)
// Calculates today + N days and stores the result in the matching NtfyData
// Next*Date field, and saves it (and N) to NTFY.json at once.
// The browser JS reloads the page after this returns 200 OK so the new date
// is immediately visible in the Parameter/Value table.
// ─────────────────────────────────────────────────────────────────────────────
void handleApplyDate(AsyncWebServerRequest *request)
{
	//SaveGenReport("handleApplyDate " + String(ESP.getFreeHeap()));
	if (request->hasParam("field") && request->hasParam("days"))
	{
		String field  = request->getParam("field")->value();
		int    days   = request->getParam("days")->value().toInt();
		String newDate = GetCurrentDate(days);
		String label  = "";

		if      (field == "pool")  { NtfyData->NextPoolBackwashDate  = newDate; NtfyData->DaysPoolBackwash = days; label = "Pool Backwash"; }
		else if (field == "tank")  { NtfyData->NextTankBackwashDate  = newDate; NtfyData->DaysTankBackwash = days; label = "Tank Backwash"; }
		else if (field == "bh")    { NtfyData->NextBHCleanDate       = newDate; NtfyData->DaysFlushHydro   = days; label = "Flushing Hydrocyclone"; }
		else if (field == "blue")  { NtfyData->NextBlueFilterDate    = newDate; NtfyData->DaysBlueFilter   = days; label = "Blue Filter"; }
		else if (field == "uv")    { NtfyData->NextUVTubeReplaceDate = newDate; NtfyData->DaysUVTube       = days; label = "UV Tube"; }
		else if (field == "chlor") { NtfyData->NextChlorineMaintDate = newDate; NtfyData->DaysChlorine     = days; label = "Pool Chlorine"; }
		NtfyData->WriteData();
		MaintainDueDatesValues();
		String fmt = newDate.substring(8,10) + "/" + newDate.substring(5,7) + "/" + newDate.substring(0,4);
		WebMessUpdate(Lastnum, label + " updated to " + fmt, "color: red", true);
	}
	sendTextResponse(request, "OK");
}

// ─────────────────────────────────────────────────────────────────────────────
// WT_ApplyDatesTable
// Returns a self-contained HTML table that lets the user type a number of days
// and click APPLY to push today + N days into the matching Next*Date field.
// APPLY saves the new date (and the days) to NTFY.json at once.
// ─────────────────────────────────────────────────────────────────────────────
void WT_ApplyDatesTable(AsyncResponseStream* response)
{
	// JavaScript — fetch /applydate; server sends status via WebSocket WebMessUpdate
	response->print(F("<script>"));
	response->print(F("function ApplyDate(field,id){"));
	response->print(F("var d=document.getElementById(id).value;"));
	response->print(F("fetch('/applydate?field='+field+'&days='+d);"));
	response->print(F("}"));
	response->print(F("</script>"));

	// Table — width:auto keeps it as narrow as content requires
	response->print(F("<div><table style='border-collapse:collapse;width:auto;font-size:14px'>"));

	// Header row
	response->print(F("<tr>"));
	response->print(F("<th style='text-align:left;padding:2px 6px'>Since Today</th>"));
	response->print(F("<th style='text-align:center;padding:2px 6px'>Days to Add</th>"));
	response->print(F("<th style='text-align:left;padding:2px 6px'>Action</th>"));
	response->print(F("</tr>"));

	// Pool Backwash
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>Pool Backwash</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='dpool' value='" + String(NtfyData->DaysPoolBackwash) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('pool','dpool')\"></td>"));
	response->print(F("</tr>"));

	// Tank Backwash
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>Tank Backwash</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='dtank' value='" + String(NtfyData->DaysTankBackwash) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('tank','dtank')\"></td>"));
	response->print(F("</tr>"));

	// Flushing Hydrocyclone
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>Flushing Hydrocyclone</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='dbh' value='" + String(NtfyData->DaysFlushHydro) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('bh','dbh')\"></td>"));
	response->print(F("</tr>"));

	// Replace Blue Filter
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>Replace Blue Filter</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='dblue' value='" + String(NtfyData->DaysBlueFilter) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('blue','dblue')\"></td>"));
	response->print(F("</tr>"));

	// UV Tube Replacement
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>UV Tube Replacement</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='duv' value='" + String(NtfyData->DaysUVTube) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('uv','duv')\"></td>"));
	response->print(F("</tr>"));

	// Pool Chlorine Maintenance
	response->print(F("<tr>"));
	response->print(F("<td style='padding:2px 6px'>Pool Chlorine Maintenance</td>"));
	response->print("<td style='padding:2px 6px;text-align:center'><input type='number' id='dchlor' value='" + String(NtfyData->DaysChlorine) + "' style='width:50px;font-size:14px'></td>");
	response->print(F("<td style='padding:2px 6px'><input type='button' value='APPLY' style='font-size:14px' onclick=\"ApplyDate('chlor','dchlor')\"></td>"));
	response->print(F("</tr>"));

	response->print(F("</table></div><div><br></div>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// appendNtfyHelp
// Explanation box at the top of the NTFY page for a first-time user (same
// style as the Rain Gauge NTFY page): phone setup, then how the maintenance
// reminders, the APPLY table and the settings below work together.
// ─────────────────────────────────────────────────────────────────────────────
static void appendNtfyHelp(AsyncResponseStream* r)
{
	// Tick box shows / hides the help; the choice is remembered in this browser
	// (localStorage) — ticked the first time, so a new user sees it at once.
	r->print(F("<div style='margin:8px 0 0;font-size:13px;text-align:left'><label>"
	           "<input type='checkbox' id='ntfyhelpcb' checked onchange='ntfyHelp(this.checked,1)'> "
	           "Show help (how NTFY works)</label></div>"
	           "<div id='ntfyhelp' style='background:#f1f5f9;border:1px solid #cbd5e1;border-radius:6px;padding:8px 10px;margin:4px 0 8px;font-size:12px;line-height:1.5;color:#334155;text-align:left'>"
	           "<b style='color:#0284c7'>Maintenance reminders on your phone with NTFY (free, no account needed)</b>"
	           "<div style='margin-top:4px'><b>1. Set up the phone</b></div>"
	           "<ol style='margin:2px 0 4px 18px;padding:0'>"
	           "<li>Install the <b>ntfy</b> app on the phone (Google Play / App Store, by Philipp Heckel).</li>"
	           "<li>Make up a <b>topic name</b>: letters, numbers, - and _ only, no spaces, e.g. <i>hydro17-smith-7342</i>. "
	           "Anyone who knows the name can read the messages, so make it hard to guess.</li>"
	           "<li>In the app tap <b>+</b> (Subscribe to topic), type <b>exactly the same name</b> "
	           "(upper / lower case matters), server <i>ntfy.sh</i>, and subscribe.</li>"
	           "<li>Type the same name in <b>NTFY Topic Name</b> below, choose the <b>Alert Level</b>, press <b>UPDATE</b>, "
	           "then <b>TEST NTFY</b> &mdash; the test message must appear on the phone.</li>"
	           "</ol>"
	           "<div><b>2. How the reminders work</b></div>"
	           "<div style='margin:2px 0 4px 6px'>"
	           "<div style='padding-left:12px;text-indent:-12px'>&bull; Each maintenance job below has a <b>Next &hellip; date</b>: pool backwash, tank backwash, flushing the "
	           "hydrocyclone, replacing the blue filter, the UV tube and pool chlorine.</div>"
	           "<div style='padding-left:12px;text-indent:-12px'>&bull; Every day at the <b>Daily Reminder Time</b> the controller checks all dates. Every job that is due "
	           "(the date is today or has passed) goes into <b>one</b> message, together with a warning when the tank "
	           "filter pressure is too high.</div>"
	           "<div style='padding-left:12px;text-indent:-12px'>&bull; The message comes <b>again every day</b> until you set a new date for that job &mdash; so it cannot be forgotten.</div>"
	           "<div style='padding-left:12px;text-indent:-12px'>&bull; <b>After doing a job:</b> in the <b>Since Today</b> table type the number of days until the next time "
	           "(e.g. 14 for the pool backwash) and press <b>APPLY</b>. The next date becomes today + those days and is saved "
	           "at once; the number of days is remembered for next time. You can also type a date straight into the "
	           "<b>Next &hellip;</b> rows and press <b>UPDATE</b>.</div>"
	           "<div style='padding-left:12px;text-indent:-12px'>&bull; The due dates are also shown on the HOME page.</div>"
	           "</div>"
	           "<b>Alert Level:</b> <i>urgent</i> = long vibration and sound, even in Do Not Disturb (if the app allows it); "
	           "<i>high</i> = sound and pop-up; <i>default</i> = normal notification; <i>low</i> = no sound; "
	           "<i>min</i> = silent, only in the list. Several phones can subscribe to the same topic."
	           "</div>"
	           "<script>function ntfyHelp(on,save){document.getElementById('ntfyhelp').style.display=on?'block':'none';"
	           "if(save){try{localStorage.setItem('ntfyHelp',on?'1':'0');}catch(e){}}}"
	           "(function(){var on=true;try{on=localStorage.getItem('ntfyHelp')!=='0';}catch(e){}"
	           "document.getElementById('ntfyhelpcb').checked=on;ntfyHelp(on,0);})();</script>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// handleNtfy  (GET /ntfy)
// Serves the NTFY configuration page HTML.
// ─────────────────────────────────────────────────────────────────────────────
void handleNtfy(AsyncWebServerRequest *request) {
	//SaveGenReport("handleNtfy " + String(ESP.getFreeHeap()));
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(Delmess, response);
	append_UL_OptionsMenu(PS_NTFY, response);
	response->print(webScriptTag);
	appendNtfyHelp(response);
	WT_ApplyDatesTable(response);
	WT_SETUP_Ntfy(response);
	response->print(webUpateButton);
	webNtfyTestButton(response);
	response->print(webHTMLEnd);
	request->send(response);
}

// ─────────────────────────────────────────────────────────────────────────────
// WT_SETUP_Ntfy
// Builds the NTFY page form table HTML using the PageBuf helper.
// ─────────────────────────────────────────────────────────────────────────────
void WT_SETUP_Ntfy(AsyncResponseStream* response) {
	resetPageBuf();
	WT_StartPage();
	WT_StartTable("Parameter  ", "Value  ", "");

	for (int i = 0; i < PAGE_VAR_NTFY; i++)
	{
		const char* typeOverride = "";
		if (i >= PAGE_VAR_NTFY_DATES_START && i < NTFY_CODE) typeOverride = "date";
		if (i == NTFY_TIME) typeOverride = "time";
		WT_EditRow(&NtfyList[i], typeOverride);
	}

	WT_EndPage();
	response->write((const uint8_t*)PageBuf, getPageBufLen());
}

// ─────────────────────────────────────────────────────────────────────────────
// MaintainNtfyPageValues  (called from MaintainWebPageValues)
// Detects changed NtfyParams values and pushes them to connected browsers.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainNtfyPageValues() {
	for (int i = 0; i < PAGE_VAR_NTFY; i++)
	{
		if (NtfyList[i].Var->GetName() != NtfyList[i].Var->GetNameLast())
		{
			webParamUpdate(NtfyList[i].Var->GetName(), NtfyList[i].Var->GetID(""), NtfyList[i].Var->GetVarStyle());
			NtfyList[i].Var->UpdateLast();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateNtfyPageValues  (called from UnpackJasonPayload on ID_NTFY)
// Applies browser form changes to NtfyParams, saves to SPIFFS, and refreshes
// the home page due-date display.
// ─────────────────────────────────────────────────────────────────────────────
void UpdateNtfyPageValues(uint32_t num, JsonObject *root) {
	for (JsonPair kv : *root)
	{
		for (int i = 0; i < PAGE_VAR_NTFY; i++)
		{
			if (NtfyList[i].Var->GetID("") == kv.key().c_str())
			{
				NtfyList[i].Var->SetName(String(kv.value().as<const char*>()));
			}
		}
	}
	WebMessUpdate(num, "Parameters Updated!", "color: red", true);
	NtfyData->WriteData();
	MaintainDueDatesValues();
}

// ─── NTFY.sh Push Notifications ───────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// NtfyTask  (permanent FreeRTOS task — Core 0, priority 1, stack 4 096 B)
// Blocks on NtfyQueue (1-second timeout).  When a message arrives it opens one
// HTTP connection to ntfy.sh, POSTs the message, then closes everything before
// looping back.  MaintainNTFY() is called on every timeout tick so the daily
// overdue-date check runs even when no manual messages are queued.
// ─────────────────────────────────────────────────────────────────────────────
void NtfyTask(void *pvParameters)
{
	Serial.flush();
	NtfyMsg msg;
	for (;;)
	{
		if (xQueueReceive(NtfyQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE)
		{
			if (OTAInProgress) continue;   // Don't compete with OTA on the WiFi stack
			WiFiClient client;
			HTTPClient http;
			String url = "http://ntfy.sh/";
			url += msg.code;
			http.setTimeout(5000);
			if (http.begin(client, url))
			{
				http.addHeader("Content-Type", "text/plain");
				http.addHeader("X-Priority",   msg.priority);
				http.POST(msg.message);
				http.end();
			}
			// http and client go out of scope here — destructors close socket cleanly
		} else {
			MaintainNTFY();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// MessageNTFY
// Copies the message into the FreeRTOS queue and returns immediately.
// If the queue is full (previous send still in progress) the message is
// silently dropped — no blocking, no busy flag needed.
// ─────────────────────────────────────────────────────────────────────────────
void MessageNTFY(String message, String priority)
{
	DEBUG_PRINTLN(F("\n--- MessageNTFY Start ---"));
	Serial.flush();

	if (NtfyQueue == NULL) {
		DEBUG_PRINTLN(F("EXIT: Queue not initialized"));
		Serial.flush();
		return;
	}

	if (WiFi.status() != WL_CONNECTED) {
		DEBUG_PRINTLN(F("EXIT: WiFi disconnected"));
		Serial.flush();
		return;
	}

	String code = NtfyData->NtfyCode;
	code.trim();
	if (code.length() == 0) {
		DEBUG_PRINTLN(F("EXIT: NtfyCode (Topic) is empty"));
		Serial.flush();
		return;
	}

	NtfyMsg msg;
	memset(&msg, 0, sizeof(NtfyMsg));

	message.toCharArray (msg.message,  sizeof(msg.message));
	priority.toCharArray(msg.priority, sizeof(msg.priority));
	code.toCharArray    (msg.code,     sizeof(msg.code));

	if (xQueueSend(NtfyQueue, &msg, 0) == pdTRUE) {
		DEBUG_PRINTLN(F("RESULT: Successfully added to Queue"));
	} else {
		DEBUG_PRINTLN(F("RESULT: FAILED - Queue is FULL"));
	}
	Serial.flush();

	delay(100); // Give serial time to transmit
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupNTFY  (called from setup() after WiFi connects)
// Creates the single-slot FreeRTOS message queue and launches NtfyTask on
// Core 0.  Must be called after WiFi is connected because NtfyTask checks
// WiFi.status() before every HTTP POST to ntfy.sh.
// ─────────────────────────────────────────────────────────────────────────────
void SetupNTFY() {
	NtfyQueue = xQueueCreate(1, sizeof(NtfyMsg));   // depth 1 — one message at a time
	BaseType_t res = xTaskCreatePinnedToCore(NtfyTask, "NtfyTask", 4096, NULL, 1, &NtfyTaskTaskHandle, 0);
	if (res == pdPASS) Serial.println(F("NtfyTask created OK"));
	else               Serial.println(F("NtfyTask creation FAILED"));
}

// ─────────────────────────────────────────────────────────────────────────────
// MaintainNTFY  (called every 1000 ms from NtfyTask)
// At the configured NtfyMessageTime (HH:MM), checks whether any of the five
// maintenance due dates are overdue and whether the tank filter pressure has
// exceeded the backwash warning threshold.  All triggered items are combined
// into one message (newline-separated) and queued once per day.  Repeats
// daily until the dates are updated / FilterPressureExceeded is cleared.
// ─────────────────────────────────────────────────────────────────────────────
void MaintainNTFY()
{
	if (NtfyQueue == NULL)                           return;
	if (WiFi.status() != WL_CONNECTED)              return;
	if (NtfyData->NtfyMessageTime.length() == 0)    return;
	String code = NtfyData->NtfyCode; code.trim();
	if (code.length() == 0)                         return;

	String currentTime = GetCurrentTime();       // "HH:MM"
	String currentDate = GetCurrentDate(0);      // "YYYY-MM-DD"

	if (currentTime != NtfyData->NtfyMessageTime) return;
	if (currentDate == NtfyLastSentDate)          return;

	// ── Collect all overdue items into one message ────────────────────────────
	String alerts = "";
	if (currentDate >= NtfyData->NextPoolBackwashDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Do Backwash - Pool Filter"; }
	if (currentDate >= NtfyData->NextTankBackwashDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Do Backwash - Tank Filter"; }
	if (currentDate >= NtfyData->NextBHCleanDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Clean Hydrocyclone - Borehole"; }
	if (currentDate >= NtfyData->NextBlueFilterDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Replace Filter - Blue"; }
	if (currentDate >= NtfyData->NextUVTubeReplaceDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Replace UV Tube"; }
	if (currentDate >= NtfyData->NextChlorineMaintDate)
		{ if (alerts.length()) alerts += "\n"; alerts += "Maintain Chlorine"; }
	if (FilterSensor != NULL && FilterSensor->FilterPressureExceeded)
		{ if (alerts.length()) alerts += "\n"; alerts += "Filter Pressure Exceeded - Tank Filter"; }

	if (alerts.length() > 0)
	{
		MessageNTFY(alerts, NtfyData->NtfyPriority);
		NtfyLastSentDate = currentDate;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// webNtfyTestButton
// Returns an HTML button that calls /testntfy via fetch() — no page reload.
// ─────────────────────────────────────────────────────────────────────────────
void webNtfyTestButton(AsyncResponseStream* r)
{
	r->print(F("<script>function ButtonNtfyTest(){fetch('/testntfy');}</script>"
	           "<div><br></div><div><b><a>"
	           "<input type=\"button\" id=\"TESTNTFY\" onclick=\"ButtonNtfyTest()\" "
	           "style=\"font-size:14px\" value=\"TEST NTFY\"></a></b></div>"));
}

// ─────────────────────────────────────────────────────────────────────────────
// handleTestNtfy  (GET /testntfy)
// Queues a test NTFY push notification, shows a status message on the
// NTFY page via WebSocket, then responds 200 OK (no redirect).
// ─────────────────────────────────────────────────────────────────────────────
void handleTestNtfy(AsyncWebServerRequest *request)
{
	//SaveGenReport("handleTestNtfy " + String(ESP.getFreeHeap()));
	MessageNTFY("TEST FROM " + Params->SysName, NtfyData->NtfyPriority);
	WebMessUpdate(Lastnum, "Test NTFY was sent!", "color: red", true);
	sendTextResponse(request, "OK");
}
