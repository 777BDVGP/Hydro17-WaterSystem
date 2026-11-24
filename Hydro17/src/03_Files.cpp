
// =============================================================================
// 03_Files.cpp — SPIFFS File System Initialisation, WiFi Credentials & File Pages
// =============================================================================
// Implements SPIFFS mount and format-on-first-boot, operational object creation,
// and all web page handlers
// for file management (Files list, Download, Upload, Delete).
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// InitFileSystem
// Mounts SPIFFS.  If /FormatComplete.txt is absent, formats the partition and
// recreates the sentinel file — this happens only on first boot or after a
// full flash erase.  Called once from setup() before objects are created.
// ─────────────────────────────────────────────────────────────────────────────
void InitFileSystem(){

	File f;

	SPIFFS.begin();
	// SPIFFS.remove(StrFormatComplete);
	if (!SPIFFS.exists(StrFormatComplete) ) {
		DEBUG_PRINTLN("Please wait 30 secs for SPIFFS to be formatted.....");
		if ( SPIFFS.format() )
		{
			DEBUG_PRINTLN("Spiffs formatted");

			f = SPIFFS.open(StrFormatComplete, "w");
			if (!f)
			{
				DEBUG_PRINTLN("file open failed 01");
			}
			else
			{
				f.println("Deleting this file will re-format SPIFFS.");
				f.close();
				DEBUG_PRINTLN("FormatComplete.txt - Created");
			}
		}
		else
		{
			DEBUG_PRINTLN("Spiffs could not format");
		}
	}
	else {
		DEBUG_PRINTLN("File System Started");		
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// InitOperationalObjects
// Creates all runtime objects (irrigation, buttons, suspension days, valves,
// schedule, settings, sensors, pool schedule) and pre-builds the home, tank,
// and suspension page-variable maps so the first HTTP request is instant.
// Called once from setup() after InitFileSystem().
// ─────────────────────────────────────────────────────────────────────────────
void InitOperationalObjects(){

	// Init Operational Objects
    CreateIrrigation();         // Create Irrigation object from "IrriObject" class
	CreateButton();				// Create Button object from "ButtonSelection" class
    CreateSuspendDays();        // Create Suspend Days object from "SuspendDays" class
    CreateElectricValves(); 	// Create Valve Objects
	CreateSheduleObj();			// Create Shedule object from "SheduleIrri" class
	CreateAllParams();          // Creates Params + calls MapSettingsPage()
	CreateNtfyParams();         // Creates NtfyData + calls MapNtfyPage()
	CreatePressureSensors();    // Create Tank Level Sensor
	CreatePoolShedule();        // Create Pool Filter Shedule
	CreatePoolLightShedule();   // Create Pool Light Shedule

	// Pre-build all page-variable maps at boot so the first HTTP request to
	// each page is as fast as subsequent ones (no lazy-init delay).
	MapHomePage();              // 08_Home.cpp   — was lazy on first / request
	MapTankPage();              // 12_Tank.cpp   — was lazy on first /tank request
	CreateSuspendMaps();        // 09_Suspend.cpp — was lazy on first /managesuspend request
}

void InitOther(){
	
	// Init Other Operational  
	UpdateTimerTimes();	
}

///////////////////////////// - WEB FILES UTILITY START - ///////////////////////////////////	

/* HTML - Directory System - START File Display*/

// -----------------------------------------------------------------------------
// FilesPage  (GET /files)
// AJAX-based file manager: usage bar, sortable file list with DL/View/Del,
// in-page viewer panel, and embedded upload form.
// File data is fetched from /api/files via JS fetch() on page load.
// -----------------------------------------------------------------------------
void FilesPage(AsyncWebServerRequest *request){
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_07, response);
	FilePageSel(PS_FILES, response);
	response->print(webScriptTag);

	// Filesystem usage bar (populated by JS)
	response->print("<div id='fsinfo' style='padding:4px 8px 2px'>"
	                "<p style='color:#475569;font-size:12px;padding:4px 0'>Loading&hellip;</p></div>");

	// File list header row
	response->print("<div style='display:flex;align-items:center;justify-content:space-between;"
	                "padding:4px 8px 2px'>"
	                "<b style='font-size:13px;color:#0f172a'>Files on SPIFFS</b>"
	                "<button onclick='loadFiles()' style='background:#0284c7;border:none;color:#fff;"
	                "padding:4px 10px;font-size:12px;border-radius:4px;cursor:pointer'>"
	                "&#8635; Refresh</button></div>");

	// File table placeholder (replaced by JS)
	response->print("<div id='ftbl'>"
	                "<p style='color:#475569;padding:10px;font-size:13px'>Loading&hellip;</p></div>");

	// File viewer panel (hidden until View is clicked)
	response->print("<div id='vpanel' style='display:none;margin:4px 0'>"
	                "<div style='display:flex;align-items:center;justify-content:space-between;"
	                "background:#e2e8f0;padding:4px 8px;border-radius:4px 4px 0 0'>"
	                "<span id='vfname' style='font-family:monospace;font-size:11px;color:#0f172a'></span>"
	                "<button onclick='closeView()' style='background:#dc2626;border:none;color:#fff;"
	                "padding:3px 8px;font-size:12px;border-radius:4px;cursor:pointer'>"
	                "&#10005; Close</button></div>"
	                "<pre id='vcontent' style='max-height:320px;overflow:auto;background:#f8fafc;"
	                "border:1px solid #e2e8f0;border-radius:0 0 4px 4px;padding:8px;"
	                "font-size:11px;white-space:pre-wrap;word-break:break-all;margin:0'></pre></div>");

	// Upload section
	response->print("<div style='padding:8px;margin-top:4px'>"
	                "<b style='font-size:13px;color:#0f172a'>Upload File</b><br><br>"
	                "<form method='POST' action='/fupload' enctype='multipart/form-data'>"
	                "<input type='file' name='fupload' "
	                "style='margin-bottom:10px;display:block;width:100%;font-size:13px'>"
	                "<button class='btn btn-bl' type='submit'>&#8679; Upload</button>"
	                "</form></div>");

	response->print(R"JS(<script>
function fmtSz(b){if(b<1024)return b+' B';if(b<1048576)return (b/1024).toFixed(1)+' KB';return (b/1048576).toFixed(1)+' MB';}
function loadFiles(){
  fetch('/api/files').then(function(r){return r.json();}).then(function(j){
    var pct=Math.round(j.used/j.total*100);
    document.getElementById('fsinfo').innerHTML=
      "<div style='padding:4px 8px 2px;font-size:12px;color:#475569'>Used: "+fmtSz(j.used)+" of "+fmtSz(j.total)+" ("+pct+"%)</div>"
      +"<div style='background:#e2e8f0;border-radius:4px;height:6px;margin:0 8px 8px'>"
      +"<div style='background:#0284c7;height:6px;border-radius:4px;width:"+pct+"%'></div></div>";
    if(!j.files||j.files.length===0){document.getElementById('ftbl').innerHTML="<p style='color:#475569;padding:10px;font-size:13px'>No files.</p>";return;}
    function extOf(n){var i=n.lastIndexOf('.');return i<0?'':n.substring(i+1).toLowerCase();}
    function nameColor(n){var e=extOf(n);return e==='json'?'#dc2626':e==='txt'?'#0f172a':'#15803d';}
    j.files.sort(function(a,b){var ea=extOf(a.name),eb=extOf(b.name);return ea<eb?-1:ea>eb?1:a.name<b.name?-1:a.name>b.name?1:0;});
    var h='<table><tr class="hd"><td>File Name</td><td style="text-align:right">Size</td><td></td></tr>';
    j.files.forEach(function(f){
      var enc=encodeURIComponent(f.name);
      var col=nameColor(f.name);
      h+='<tr><td style="font-family:monospace;font-size:12px;color:'+col+'">'+f.name+'</td>'
        +'<td style="font-size:12px;color:#64748b;white-space:nowrap;text-align:right">'+fmtSz(f.size)+'</td>'
        +'<td style="white-space:nowrap;padding-right:0;text-align:right">'
        +'<a href="/download?f_File='+enc+'" style="display:inline-block;padding:3px 8px;background:#0284c7;color:#fff;border-radius:4px;font-size:11px;text-decoration:none;margin-right:3px">&#8595;&nbsp;DL</a>'
        +'<button onclick="viewFile(\''+f.name+'\')" style="padding:3px 8px;background:#475569;color:#fff;border:none;border-radius:4px;font-size:11px;cursor:pointer;margin-right:3px">&#128065;&nbsp;View</button>'
        +'<button onclick="delFile(\''+f.name+'\')" style="padding:3px 8px;background:#dc2626;color:#fff;border:none;border-radius:4px;font-size:11px;cursor:pointer">&#10005;&nbsp;Del</button>'
        +'</td></tr>';
    });
    h+='</table>';
    document.getElementById('ftbl').innerHTML=h;
  }).catch(function(){document.getElementById('ftbl').innerHTML="<p style='color:#dc2626;padding:10px;font-size:13px'>Failed to load.</p>";});
}
function delFile(name){
  if(!confirm('Delete '+name+'?\nThis cannot be undone.'))return;
  fetch('/files/delete?f='+encodeURIComponent(name)).then(function(r){return r.json();})
    .then(function(j){if(j.ok)loadFiles();else alert('Error: '+j.msg);});
}
function viewFile(name){
  document.getElementById('vcontent').textContent='Loading...';
  document.getElementById('vfname').textContent=name;
  document.getElementById('vpanel').style.display='block';
  document.getElementById('vpanel').scrollIntoView({behavior:'smooth'});
  fetch('/files/view?f='+encodeURIComponent(name))
    .then(function(r){return r.text();})
    .then(function(t){document.getElementById('vcontent').textContent=t;})
    .catch(function(){document.getElementById('vcontent').textContent='(error loading file)';});
}
function closeView(){document.getElementById('vpanel').style.display='none';document.getElementById('vcontent').textContent='';}
loadFiles();
</script>)JS");

	response->print(webHTMLEnd);
	request->send(response);
}

// Page Main Variables
void FilePageSel(int Index, AsyncResponseStream* r){
	r->print(F("<div class='nav'>"));
	r->print((Index == PS_FILES_HOME)  ? "<a class='act'>HOME</a>"  : "<a href='/'>HOME</a>");
	r->print((Index == PS_FILES_OTHER) ? "<a class='act'>OTHER</a>" : "<a href='/other'>OTHER</a>");
	r->print((Index == PS_FILES)       ? "<a class='act'>FILES</a>" : "<a href='/files'>FILES</a>");
	r->print(F("</div>"));
}

// -----------------------------------------------------------------------------
// handleApiFiles  (GET /api/files)
// Returns JSON with SPIFFS usage stats and array of all files.
// Uses PageBuf (8 KB static) + snprintf — zero heap for JSON building.
// -----------------------------------------------------------------------------
void handleApiFiles(AsyncWebServerRequest *request) {
	//SaveGenReport("handleApiFiles " + String(ESP.getFreeHeap()));
	resetPageBuf();
	char tmp[80];
	snprintf(tmp, sizeof(tmp), "{\"used\":%u,\"total\":%u,\"files\":[",
	         (unsigned)SPIFFS.usedBytes(), (unsigned)SPIFFS.totalBytes());
	appendToPageBuf(tmp);
	File root = SPIFFS.open("/");
	File f    = root.openNextFile();
	bool first = true;
	while (f) {
		const char* raw   = f.name();
		const char* fname = (raw && raw[0] == '/') ? raw + 1 : raw;
		if (fname && IsProtectedFile(fname)) { f = root.openNextFile(); continue; }
		if (!first) appendToPageBuf(",");
		snprintf(tmp, sizeof(tmp), "{\"name\":\"%s\",\"size\":%u}",
		         fname ? fname : "", (unsigned)f.size());
		appendToPageBuf(tmp);
		first = false;
		f = root.openNextFile();
	}
	root.close();
	appendToPageBuf("]}");
	getPageBufLen();
	request->send(200, "application/json", PageBuf);
}

// -----------------------------------------------------------------------------
// handleFileView  (GET /files/view?f=<filename>)
// Serves the named SPIFFS file as text/plain for the in-page viewer.
// -----------------------------------------------------------------------------
void handleFileView(AsyncWebServerRequest *request) {
	//SaveGenReport("handleFileView " + String(ESP.getFreeHeap()));
	if (!request->hasArg("f")) { request->send(400, "text/plain", "Missing filename"); return; }
	String path = "/" + request->arg("f");
	if (IsProtectedFile(path)) { request->send(403, "text/plain", "Protected file"); return; }
	if (!SPIFFS.exists(path)) { request->send(404, "text/plain", "Not found"); return; }
	request->send(SPIFFS, path, "text/plain");
}

// -----------------------------------------------------------------------------
// handleFileDeleteJson  (GET /files/delete?f=<filename>)
// Deletes the named file and returns JSON result for the AJAX caller.
// -----------------------------------------------------------------------------
void handleFileDeleteJson(AsyncWebServerRequest *request) {
	//SaveGenReport("handleFileDeleteJson " + String(ESP.getFreeHeap()));
	if (!request->hasArg("f")) {
		request->send(200, "application/json", "{\"ok\":false,\"msg\":\"missing\"}");
		return;
	}
	String path = "/" + request->arg("f");
	if (!SPIFFS.exists(path)) {
		request->send(200, "application/json", "{\"ok\":false,\"msg\":\"not found\"}");
		return;
	}
	SPIFFS.remove(path)
		? request->send(200, "application/json", "{\"ok\":true}")
		: request->send(200, "application/json", "{\"ok\":false,\"msg\":\"delete failed\"}");
}

/* HTML - Directory System - END File Display*/

/* HTML - Directory System - START File Download*/

void File_Download(AsyncWebServerRequest *request){
	if (request->args() > 0 && request->hasParam("f_File")){
		DownloadFile(request, request->getParam((size_t)0)->value());
	}
	else{
		CompilePage_Download(request);
	}
}

void CompilePage_Download(AsyncWebServerRequest *request){
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_02, response);
	FilePageSel(PS_FILES_DOWNLOAD, response);
	response->print(webScriptTag);
	append_Form_Download(response);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_Download(AsyncResponseStream* r)
{
 	CreateDFileList("/");
	r->print(F("<form><table>"));
	for (int y = 0; y < StructIdx; y++) {
		String FileName = String(SortedFiles[y].FileName);
		OP01 = "/download?f_File="; OP01 += FileName; OP01 += "'>";
		OP03 = FileName;
		r->print(F("<tr><th><p><lk><a href='"));
		r->print(OP01);
		r->print(OP03);
		r->print(F("</a></lk></p></th></tr>"));
	}
	r->print(F("</table></form>"));
}

void DownloadFile(AsyncWebServerRequest *request, String filename)
{
	String path = "/" + filename;
	if (SPIFFS.exists(path) && !IsProtectedFile(path)){
		request->send(SPIFFS, path, "application/octet-stream", true);
	}
	else{
		SetMess    = FA_MESS_RED;
		SpesialStr = "File "; SpesialStr += filename; SpesialStr += " not Available";
		CompilePage_Download(request);
	}
}

/* HTML - Directory System - END File Download*/

/* HTML - Directory System - START File Upload*/

void File_Upload(AsyncWebServerRequest *request){
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_03, response);
	FilePageSel(PS_FILES_UPLOAD, response);
	response->print(webScriptTag);
	response->print(F("<h3>  </h3>"));
	response->print(F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>"));
	response->print(F("<input class='buttons' style='width:90%' type='file' name='fupload' id='fupload' value=''><br>"));
	response->print(F("<br><button class='buttons' style='width:50%' type='submit'>Upload File</button><br><br>"));
	response->print(webHTMLEnd);
	request->send(response);
}

File UploadFile;
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
	if (index == 0){
		//SaveGenReport("handleFileUpload " + String(ESP.getFreeHeap()));
		if (!filename.startsWith("/")) filename = "/" + filename;
		SPIFFS.remove(filename);                   // Remove any previous version
		UploadFile = SPIFFS.open(filename, "w");   // Open for writing
	}

	if (UploadFile) UploadFile.write(data, len);   // Append chunk to file

	if (final){
		if (UploadFile){
			UploadFile.close();
			SetMess    = FA_MESS_GRN;
			SpesialStr = "File "; SpesialStr += filename; SpesialStr += " Uploaded";
		}
		else{
			SetMess    = FA_MESS_RED;
			SpesialStr = "File not Uploaded";
		}
		// Redirect to /files is handled by the onRequest lambda registered in AddRoutes()
	}
}

/* HTML - Directory System - END File Upload*/

/* HTML - Directory System - START File Delete*/  

void File_Delete(AsyncWebServerRequest *request){
	if (request->args() > 0){
		SPIFFS_file_delete(request, request->getParam((size_t)0)->value());
	}
	else{
		Compile_PageDelete(request);
	}
}

void Compile_PageDelete(AsyncWebServerRequest *request)
{
	AsyncResponseStream *response = beginHtmlStream(request);
	if (!response) return;
	response->print(webHeadPage);
	append_FileOption(FA_MESS_02, response);
	FilePageSel(PS_FILES_DELETE, response);
	response->print(webScriptTag);
	append_Form_Delete(response);
	response->print(webHTMLEnd);
	request->send(response);
}

void append_Form_Delete(AsyncResponseStream* r)
{
	CreateDFileList("/");
	r->print(F("<form><table>"));
	for (int y = 0; y < StructIdx; y++) {
		String FileName = String(SortedFiles[y].FileName);
		OP01 = "/delete?f_File="; OP01 += FileName; OP01 += "'>";
		OP03 = FileName;
		r->print(F("<tr><th><p><lk><a href='"));
		r->print(OP01);
		r->print(OP03);
		r->print(F("</a></lk></p></th></tr>"));
	}
	r->print(F("</table></form>"));
}

void SPIFFS_file_delete(AsyncWebServerRequest *request, String filename)
{
	File dataFile = SPIFFS.open("/" + filename, "r");
	if (dataFile){
		dataFile.close();
		if (SPIFFS.remove("/" + filename)){
			SetMess    = FA_MESS_GRN;
			SpesialStr = "File "; SpesialStr += filename; SpesialStr += " has been erased";
		}
		else{
			SetMess    = FA_MESS_RED;
			SpesialStr = "File "; SpesialStr += filename; SpesialStr += " was not deleted";
		}
	}
	else{
		SetMess    = FA_MESS_RED;
		SpesialStr = "File "; SpesialStr += filename; SpesialStr += " Not Present";
	}
	Compile_PageDelete(request);
}

/* HTML - Directory System - END File Delete*/

/* CreateDFileList - START */

void CreateDFileList(const char * dir)
{
//  List All Files In directory	
	StructIdx = 0;	
	File root = SPIFFS.open(dir);
	File file = root.openNextFile();
	while(file > 0){	
		String FileName = file.name(); 
		int Start = FileName.indexOf(".") + 1;	
		String SortName = FileName.substring(Start,String(file.name()).length());
		SortName += String(file.name()).substring(0,Start);	
		if ( StructIdx < File_array_LEN && !IsProtectedFile(FileName) )
		{		
			FileName.toCharArray(SortedFiles[StructIdx].FileName, FileName.length()+1); 	
			SortName.toCharArray(SortedFiles[StructIdx].SortName, SortName.length()+1); 	
 			SortedFiles[StructIdx].FileSize = file.size();  
			StructIdx++;					
		}
		file = root.openNextFile();	 
	}
 
//  Sort Array Alphabetically
    for ( int x = StructIdx ; x > 0; x-- )
	{	
		for ( int y = 0 ; y < (x - 1); y++ )
		{
			String TOPS = String(SortedFiles[y].SortName);
			String BOTS = String(SortedFiles[y+1].SortName);
			if (TOPS > BOTS)
			{
	 			int TOPZ = SortedFiles[y].FileSize;	
			    String TOPF = String(SortedFiles[y].FileName);
			    String BOTF = String(SortedFiles[y+1].FileName);				
				BOTS.toCharArray(SortedFiles[y].SortName, BOTS.length()+1);				
				BOTF.toCharArray(SortedFiles[y].FileName, BOTF.length()+1);
 				SortedFiles[y].FileSize = SortedFiles[y+1].FileSize;
				
				TOPS.toCharArray(SortedFiles[y+1].SortName, TOPS.length()+1);
				TOPF.toCharArray(SortedFiles[y+1].FileName, TOPF.length()+1);				
 				SortedFiles[y+1].FileSize = TOPZ;			
			}	
		}
		
	}
}
/* CreateDFileList - END */

///////////////////////////// - WEB FILES UTILITY END - ///////////////////////////////////
