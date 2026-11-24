// =============================================================================
// 14_Plugs.cpp — General Purpose Plugs Relay Page Handler
// =============================================================================
// Serves the /plugs page which allows manual control of the plugs relay
// (O_PLUGS_SWITCH_RELAY).  Automatic scheduling is not yet implemented.
// The physical relay is also controlled by the I_PLUGS_ON_OFF panel switch
// which is debounced and reflected in InPort[I_PLUGS_ON_OFF].Input.
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// handlePlugs  (GET /plugs)
// Serves the Plugs relay control page.
// ─────────────────────────────────────────────────────────────────────────────
void handlePlugs(AsyncWebServerRequest *request){
	//SaveGenReport("handlePlugs " + String(ESP.getFreeHeap()));
	CompilePage_Plugs(request);
}

void CompilePage_Plugs(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_06, response);
	appendMainPageSel(PS_PLUGS, response);
	append_Form_Plugs(response);
	response->print(webScriptTag);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_Plugs(AsyncResponseStream* response)
{
} 
///////////////////////////////////////////////////////////////////////////////////////////