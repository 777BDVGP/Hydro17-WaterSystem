// =============================================================================
// 04_WebPages.h — Web Server, WebSocket, HTML Templates & PageElement Class
// =============================================================================
// Declares the AsyncWebServer and AsyncWebSocket instances, defines all shared
// HTML/CSS/JavaScript string constants (stored in PROGMEM), and implements the
// PageElement class that binds C++ variables to browser form elements for
// real-time WebSocket updates.
//
// Architecture:
//   - The HTTP server and WebSocket both run on port 80 via ESPAsyncWebServer.
//   - Each page builds its HTML from the shared webHeadPage / webSocketScript /
//     webHTMLEnd PROGMEM fragments plus page-specific content.
//   - PageElement instances are grouped into PageVar arrays (one per page).
//     MaintainWebService iterates over these arrays and pushes any changed values
//     to connected browsers as JSON via the WebSocket.
// =============================================================================

// ─── Web Page Variable ID Base Offsets ────────────────────────────────────────
// IDs are assigned contiguously within each page's range.
// The base value ensures no two pages share an element ID.
    #define ID_HOME  		 50
    #define ID_SETTINGS		100
    #define ID_SUSPEND		150
    #define ID_VALVES		200
    #define ID_VALCONF		250
    #define ID_IRRISETUP    300
    #define ID_TANK 		350
    #define ID_POOL 		400
    #define ID_POOL_LIGHT   422
    #define ID_ABOUT        450
    #define ID_NTFY         470

// ─── Web Server & WebSocket ───────────────────────────────────────────────────
// Both run on port 80 — the WebSocket endpoint is at ws://<ip>/ws.
	AsyncWebServer server(80);
	AsyncWebSocket ws("/ws");

// ─── Theme Colour Constants ───────────────────────────────────────────────────
// CSS colour strings injected into page element style attributes via WebSocket.
	String COLOR_DEFAULT = "color:#227c91";   // Default teal label colour
	String COLOR_RED     = "color:#ff0000";   // Alert / error
	String COLOR_BLUE    = "color:#3366ff";   // Information
	String COLOR_GREEN   = "color:#33cc33";   // Success / active
	String COLOR_VIOLET  = "color:#ff00ff";   // Warning / special

// ─── append_FileOption Message Codes ─────────────────────────────────────────
// Passed to append_FileOption() to select which status message to display.

    #define FA_MESS_01		1   //  Files on SPIFFS
    #define FA_MESS_02		2   //  Click on File to Download	
    #define FA_MESS_03		3   //  Upload a File	
    #define FA_MESS_04		4   //  Click on File to Delete
    #define FA_MESS_06		6   //  Selection Option	
    #define FA_MESS_07		7   //  Files on SPIFFS
    #define FA_MESS_08	    8   //  Select Report
    #define FA_MESS_09	    9   //  File Not Found	
    #define FA_MESS_10	   10   //  Del Genreport	
    #define FA_MESS_11	   11   //  Del	PumpRep	
    #define FA_MESS_RED	   12   //  SpesialStr RED
    #define FA_MESS_GRN	   13   //  SpesialStr GREEN	

	int Delmess = FA_MESS_06;
	#define DEL_FNF        9 
	#define DEL_GEN        10 	
	#define DEL_PUMP       11 		
	
// ─── Connection Tracking ──────────────────────────────────────────────────────
	struct Connections
	{
		bool Status;  // true = client connected; false = disconnected
	};

	uint32_t Lastnum        = 0;     // WebSocket client ID of the most recently active client
	bool     LastnumUpdated = true;  // true = Lastnum has been refreshed since last use

// ─── HTML Page Fragments (PROGMEM) ────────────────────────────────────────────
// These three fragments bracket every page: webHeadPage opens the <html>, adds
// the CSS stylesheet, and ends before <body>; webSocketScript injects the
// JavaScript WebSocket client; webHTMLEnd closes </body></html>.

// webStyleSheet — CSS rules written to SPIFFS as /style.css at boot.
// Pages reference it via a <link> in webHeadPage with a ?v=__DATE__ cache-buster.
// Browser caches 24 hours; URL changes on every firmware build so fresh CSS is
// always fetched after OTA.  Storing CSS in SPIFFS eliminates the ~3500-byte
// inline style block that forced AsyncResponseStream cbuf to resize on every
// page load, fragmenting the DRAM heap.
char webStyleSheet[] PROGMEM = R"=====(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#f8fafc;color:#0f172a;max-width:600px;margin:0 auto;padding:8px;font-size:13px}
h2{font-size:20px;font-weight:800;letter-spacing:1px;color:#0284c7;text-align:center;padding:10px 0 2px}
h4{font-size:12px;color:#475569;text-align:center;margin:2px 0}
p{text-align:left;margin-left:8px;margin-top:0;margin-bottom:0}
ul{list-style-type:none;margin:4px 0;padding:0;overflow:hidden;background:#e0f2fe;border-radius:4px}
li{float:left;border-right:1px solid #bae6fd}
li a{display:block;color:#0369a1;padding:6px 10px;text-decoration:none;font-size:12px}
li a:hover{background:#bae6fd}
lk{float:left;background:#e0f2fe;font-size:12px}
lk a{display:block;color:#0284c7;padding:4px 8px;text-decoration:none}
lk a:hover{background:#bae6fd}
.nav{display:flex;flex-wrap:wrap;border:1px solid #0284c7;border-radius:6px;overflow:hidden;margin:6px 0}
.nav a{flex:1 1 auto;min-width:55px;padding:10px 4px;background:#bae6fd;color:#0369a1;text-align:center;text-decoration:none;font-size:12px;font-weight:700;border-right:1px solid #0284c7;white-space:nowrap}
.nav a:last-child{border-right:none}
.nav a:hover:not(.act):not(.warn):not(.dis){background:#7dd3fc}
.nav a.act{background:#0284c7;color:#fff}
.nav a.warn{background:#dc2626;color:#fff}
.nav a.warn:hover{background:#b91c1c}
.nav a.dis{background:#f1f5f9;color:#94a3b8;cursor:default;pointer-events:none}
table{width:100%;border-collapse:collapse;font-size:13px;margin:4px 0}
.hd td,.hd th{color:#64748b;font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;border-bottom:2px solid #cbd5e1;padding:4px 6px;text-align:left}
.hd th:last-child,.hd td:last-child{text-align:right}
td,th{padding:5px 6px;border-bottom:1px solid #e2e8f0;word-break:break-word;overflow-wrap:break-word}
tr:not(.hd) td:first-child{width:100%}
tr:not(.hd) td:last-child{text-align:right;white-space:nowrap;padding-right:0}
tr:not(.hd) td:last-child label{display:inline-block;vertical-align:middle}
tr:not(.hd) th:first-child{width:100%}
tr:not(.hd) th:last-child:not(:first-child){text-align:right;white-space:nowrap;padding-right:0}
tr:not(.hd) th:last-child:not(:first-child) p{text-align:right;margin-left:0}
table.grid tr:not(.hd) td:first-child{width:auto;text-align:left}
table.grid tr:not(.hd) td:not(:first-child){padding-left:10px;padding-right:10px;text-align:center}
table.grid tr:not(.hd) td:last-child{text-align:center;white-space:normal;padding-right:10px}
table.grid tr:not(.hd) td{text-align:center}
table.grid tr th{text-align:center;white-space:nowrap;padding-left:10px;padding-right:10px}
table.grid tr th:first-child{text-align:left;white-space:normal;padding-left:6px;padding-right:6px}
table.grid tr th:last-child:not(:first-child){text-align:center;white-space:nowrap;padding-right:10px}
table.grid .hd th{text-align:center}
table.grid .hd th:first-child{text-align:left}
table.grid .hd th:last-child{text-align:center}
table.susp th:not(:first-child){min-width:65px;text-align:center}
table.susp tr:not(.hd) td:not(:first-child){min-width:65px;text-align:center}
input[type=time],input[type=text],input[type=number],input[type=date]{background:#fff;border:1px solid #cbd5e1;border-radius:4px;color:#0f172a;padding:3px 5px;font-size:12px;width:120px;max-width:45vw}
select{background:#fff;border:1px solid #cbd5e1;border-radius:4px;color:#0f172a;padding:3px 5px;font-size:12px;width:120px;max-width:45vw}
input[type=checkbox]{accent-color:#0284c7;cursor:pointer;width:16px;height:16px}
input[type=time]:focus,input[type=text]:focus,input[type=number]:focus,input[type=date]:focus,select:focus{outline:1px solid #0284c7}
input[type=button],input[type=submit]{background:#0284c7;border:none;color:#fff;padding:5px 14px;font-size:13px;cursor:pointer;border-radius:4px}
input[type=button]:hover,input[type=submit]:hover{background:#0369a1}
button{background:#0284c7;border:none;color:#fff;padding:5px 14px;font-size:13px;cursor:pointer;border-radius:4px}
button:hover{background:#0369a1}
.btn{display:inline-block;padding:7px 18px;border-radius:5px;font-size:13px;font-weight:700;border:none;cursor:pointer;margin:4px;text-decoration:none}
.btn-bl{background:#1d4ed8;color:#fff}.btn-bl:hover{background:#2563eb}
label{color:#0f172a}
)=====";

// webHeadPage — Minimal HTML head section (~160 bytes, required on every page).
// CSS is loaded from the browser-cached /style.css static file instead of being
// inlined, eliminating the ~3500-byte cbuf resize that fragmented the DRAM heap.
char webHeadPage[] PROGMEM =
	"<html><head>"
	"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
	"<link rel=\"icon\" href=\"data:,\">"
	"<link rel=\"stylesheet\" href=\"/style.css?v=" __DATE__ __TIME__ "\">"
	"</head>"
	"<body onload=\"javascript:init()\">";

// Script WebSocket — raw JS, written to SPIFFS as /wss.js at boot.
// Pages reference it via webScriptTag which embeds the build date as a
// cache-buster query parameter (?v=__DATE__).  The browser caches the file
// for 24 hours, but the URL changes with every firmware build so the browser
// always fetches fresh JS after an OTA update.
char webSocketScript[] PROGMEM = R"=====(
	function init() {
		StartWebSocket();
	}

	// OTA live status helper — updates the ota_status input on the About page
	function setOtaStatus(msg, color) {
		var el = document.getElementById("ota_status");
		if (el) { el.value = msg; el.style.color = color; }
	}

	var Socket;
	function StartWebSocket() {
		Socket = new WebSocket('ws://' + window.location.host + '/ws');

		Socket.onopen = function(event) {};

		Socket.onerror = function(event) {
			console.log("Socket.onerror");
			setTimeout(StartWebSocket, 300);
		};

		Socket.onclose = function(event) {
			console.log("Socket.onclose");
		};

		// Update web page elements from ESP32
		Socket.onmessage = function(event) {
			const jsonObject = JSON.parse(event.data);

			// Handle OTA-specific messages — update About page status live
			if (jsonObject.type === "ota_start")    { setOtaStatus("Upload starting...", "orange"); return; }
			if (jsonObject.type === "ota_progress") { setOtaStatus("Uploading... " + jsonObject.percent + "%", "orange"); return; }
			if (jsonObject.type === "ota_end")      { setOtaStatus("Busy rebooting...", "red"); setTimeout(function(){ location.reload(); }, 5000); return; }
			if (jsonObject.type === "ota_error")    { setOtaStatus("Update failed!", "red"); return; }

			var ElementAtt;
			for (const [key, value] of Object.entries(jsonObject)) {
				if (key === "Attribute") {
					const attrName = jsonObject["AttributeName"] || "style";
					if (ElementAtt !== null) { ElementAtt.setAttribute(attrName, value); }
				} else if (key === "AttributeName") {
					// consumed by the "Attribute" handler above
				} else {
					const Element = document.getElementById(key);
					if (Element === null) { ElementAtt = null; continue; }
					if (Element.tagName === 'SELECT') {
						Element.value = value;
						ElementAtt = Element;
					} else {
						if (Element.type === "checkbox") {
							Element.checked = (value === "true");
							ElementAtt = null;
						} else {
							Element.value = value;
							Element.innerHTML = value;
							ElementAtt = Element;
						}
					}
				}
			}
		}
	}

	// Read all inputs and send to ESP32 via WebSocket
	function UpdatePage() {
		const jsonObject = {};
		for (const input of document.querySelectorAll("input")) {
			jsonObject[input.id] = (input.type === "checkbox") ? (input.checked ? "true" : "false") : input.value;
		}
		for (const select of document.querySelectorAll("select")) {
			jsonObject[select.id] = select.value;
		}
		Socket.send(JSON.stringify(jsonObject));
	}

	function ButtonUser1() {
		Socket.send(JSON.stringify({"MAN1": "User 1"}));
	}

	function ButtonUser2() {
		Socket.send(JSON.stringify({"MAN2": "User 2"}));
	}

	function ButtonReboot() {
		Socket.send(JSON.stringify({"REBOOT": "YES"}));
	}

	// Build label+input table below header
	function createLabelInputTable(parameters, tableClass) {
		const tableElement = document.createElement("table");
		if (tableClass) tableElement.className = tableClass;
		const headerRow = document.createElement("tr");

		for (const parameter of parameters) {
			if (!parameter.name) continue;
			const headerCell = document.createElement("th");
			headerCell.textContent = parameter.name;
			headerRow.appendChild(headerCell);
		}

		if (headerRow.textContent.trim() !== '') {
			headerRow.querySelectorAll('th').forEach(cell => { cell.style.textAlign = 'left'; });
			tableElement.appendChild(headerRow);
		}

		for (const parameter of parameters) {
			if (parameter.name !== undefined) continue;
			if (parameter.sectionHeader !== undefined) {
				const row = document.createElement("tr");
				const cell = document.createElement("td");
				cell.colSpan = 2;
				cell.textContent = parameter.sectionHeader;
				cell.style.cssText = "font-weight:bold;padding:4px 6px;text-align:left;";
				row.appendChild(cell);
				tableElement.appendChild(row);
				continue;
			}
			const row = document.createElement("tr");

			for (const label of (parameter.labels || [])) {
				const cell = document.createElement("td");
				const labelElement = document.createElement("label");
				labelElement.textContent = label.text;
				labelElement.id = label.id;
				for (const a of (label.attributes || [])) { labelElement.setAttribute(a.n, a.v); }
				cell.appendChild(labelElement);
				row.appendChild(cell);
			}

			// Build one input element from a descriptor object
				const makeInputEl = (input) => {
					if (input.type === "select") {
						const el = document.createElement("select");
						el.id = input.id;
						for (const a of (input.attributes || [])) { el.setAttribute(a.n, a.v); }
						for (const optionText of input.options) {
							const opt = document.createElement("option");
							opt.value = optionText;
							opt.textContent = optionText;
							el.appendChild(opt);
						}
						el.value = input.value;
						return el;
					} else {
						const el = document.createElement("input");
						el.type = input.type;
						el.id = input.id;
						el.value = input.value;
						if (input.type === "checkbox") { el.checked = (input.value === "true"); }
						for (const a of (input.attributes || [])) { el.setAttribute(a.n, a.v); }
						return el;
					}
				};
				// grouped:true → all inputs share one <td> (prevents extra columns)
				const inputItems = parameter.inputs || [];
				if (parameter.grouped && inputItems.length > 1) {
					const cell = document.createElement("td");
					for (const input of inputItems) { cell.appendChild(makeInputEl(input)); }
					row.appendChild(cell);
				} else {
					for (const input of inputItems) {
						const cell = document.createElement("td");
						cell.appendChild(makeInputEl(input));
						row.appendChild(cell);
					}
				}

			tableElement.appendChild(row);
		}

		document.body.appendChild(tableElement);
	}

)=====";
// Script WebSocket - END

// ─── WebSocket Script Page Tag ────────────────────────────────────────────────
// Generates a <script src='/wss.js?v=DATE'> tag at compile time.
// __DATE__ (e.g. "Apr 16 2026") changes with every firmware build, so the
// browser treats each build as a new URL and fetches fresh JS after OTA.
// Within the same build the browser serves it from its 24-hour cache.
	#define webScriptTag "<script src='/wss.js?v=" __DATE__ __TIME__ "'></script>"

char webHTMLEnd[] PROGMEM = R"=====( 
</body>
</html>
)=====";

char webUpateButton[] PROGMEM = R"=====( 
	<div><br></div> 
	<div>
 		<b><a><input type="button" id="Update" onclick="UpdatePage()" value="UPDATE" style="font-size:14px" ></a></b>
	</div>  	
)=====";

// =============================================================================
// SelectOptions
// =============================================================================
// Stores one option string for a <select> dropdown bound to a PageElement.
// Options are added via PageElement::AddOption() and retrieved via GetOption().
// =============================================================================
class SelectOptions
{
public:
 	String Option = "";  // Option text shown in the <select> dropdown
};

// =============================================================================
// PageElement
// =============================================================================
// Binds a single C++ variable (String, double, float, int, or bool) to a
// browser form element.  Each instance stores:
//   - A pointer to the live application variable
//   - A snapshot of the last-sent value (used to detect changes for WebSocket push)
//   - A fixed style string (label column CSS)
//   - A pointer to a variable style string (value column CSS, dynamically set)
//   - An integer ID that maps to an HTML element id="<prefix><ID>"
//
// For String types, SetStringToSelect() converts the field to a <select>
// dropdown with options added via AddOption().
//
// Typical usage:
//   new PageElement(&myVar, "Label Text", "", &myStyleStr, ID_BASE + index);
// =============================================================================

// ─── PageElement Variable Type Constants ─────────────────────────────────────
	#define VTYPE_STRING  1   // Bound variable is a String
	#define VTYPE_DOUBLE  2   // Bound variable is a double
	#define VTYPE_FLOAT   3   // Bound variable is a float
	#define VTYPE_INT     4   // Bound variable is an int
	#define VTYPE_BOOL    5   // Bound variable is a bool

	class PageElement
	{
	public:
		void* operator new(size_t sz)  { void* p = ps_malloc(sz); return p ? p : malloc(sz); }
		void  operator delete(void* p) { free(p); }
		// Constructors
		PageElement(String* InName, String Desc, String FStyle, String* VStyle, int ID);    		
		PageElement(double* InName, String Desc, String FStyle, String* VStyle, int ID);    			
		PageElement(float* InName,  String Desc, String FStyle, String* VStyle, int ID);    	
		PageElement(int* InName,    String Desc, String FStyle, String* VStyle, int ID);    	
 		PageElement(bool* InName,   String Desc, String FStyle, String* VStyle, int ID);    	
		// Destructor
		~PageElement();   
	 
		// Methods 
		void   SetName(String InName);    	    // To Update the Actual value of the referencing field		 	
		String GetName();                       // Get the contents of the referencing Object as a String
		const String& GetVarStyle();		            // Get Style referencing Field
		String GetNameLast();	                // Get the Last name
		void   UpdateLast();		            // Update the Last Name
		void   ClearLast();		                // Force re-push: resets last value so change is detected
		const String& GetDescription();                // Get Description of Referencing Field
		const String& GetFStyle();                     // Get Style of Description of Ref Field
		String GetTypeStr();                    // Get the Type of variable
		String GetID(const char* Type);              // Get The ID
		void   SetStringToSelect(int Options);  // Set Field as select and amount of options
		void   AddOption(String Option);        // Add a Option
		const String& GetOption(int Idx);              // Get Options
		int           GetOptionCount();
		bool          HasChanged();
		void          SetTextAlign(String align);
		void          SetRange(float mn, float mx, float step = NAN);
		void          SetPlaceholder(String ph);
		void          SetReadOnly(bool ro = true);
		const String& GetAttrTextAlign();
		bool HasAttrMin(); bool HasAttrMax(); bool HasAttrStep();
		float GetAttrMin(); float GetAttrMax(); float GetAttrStep();
		const String& GetAttrPlaceholder();
		bool GetAttrReadOnly();

	private:
	    // Variables
		String* NameS;                  // Reference of a String
		double* NameD;                  // Reference of a Double
		float*  NameF;                  // Reference of a Float
		int*    NameI;                  // Reference of a Int
		bool*   NameB;	                // Reference of a Bool
		int 	Type;                   // Defining the variable type created in this Object
		
		String NameSLast;               // Previous value of referencing field String
		double NameDLast;               // Previous value of referencing field Double
		float  NameFLast;               // Previous value of referencing field Float
		int    NameILast;               // Previous value of referencing field Int
		String NameBLast;	            // Previous value of referencing field Bool
		                                 
		String  VarDesc = "";		    // Description on WEB page of the referencing field
		String  FixStyle = "";	        // Style used on WEB Page on Description of Ref Field
		String* VarStyle;               // Style used on WEB Page of the referencing Field
		int  VarID = 0;                 // ID of Field on WEB Page
		bool NameStringSelect = false;  // Indication of String field "text" or "Select" (Options)
		int  OptionCount = 0;           // Number of Options created
		int  OptionIndex = 0;           // Index of Option for Adding or Getting
		SelectOptions* OptionList = nullptr;
		String AttrTextAlign = "";
		float AttrMin = NAN; float AttrMax = NAN; float AttrStep = NAN;
		String AttrPlaceholder = ""; bool AttrReadOnly = false;
	};
//	
//   Constructor for String
    PageElement::PageElement(String* InName, String Desc, String FStyle, String* VStyle, int ID){	
 		NameS = InName;
		NameSLast = *NameS;		
		Type = VTYPE_STRING;
		VarDesc = Desc;
		FixStyle = FStyle;
		VarStyle = VStyle;
		VarID = ID;
	} 	
	
//  Constructor for double	
    PageElement::PageElement(double* InName, String Desc, String FStyle, String* VStyle,int ID){		
 		NameD = InName;
		NameDLast = *NameD;			
		Type = VTYPE_DOUBLE;	
		VarDesc = Desc;	
		FixStyle = FStyle;	
		VarStyle = VStyle;		
		VarID = ID;		
	}	
	
//   Constructor for float	
    PageElement::PageElement(float* InName, String Desc, String FStyle, String* VStyle,int ID){		
		NameF = InName;		
		NameFLast = *NameF;		
		Type = VTYPE_FLOAT;	
		VarDesc = Desc;	
		FixStyle = FStyle;	
		VarStyle = VStyle;		
		VarID = ID;		
	}
	
//   Constructor for int 	
    PageElement::PageElement(int* InName, String Desc, String FStyle, String* VStyle,int ID){	
		NameI = InName;	
		NameILast = *NameI;			
		Type = VTYPE_INT;	
		VarDesc = Desc;	
		FixStyle = FStyle;
		VarStyle = VStyle;		
		VarID = ID;		
	}	
	
//   Constructor for boolean	
    PageElement::PageElement(bool* InName, String Desc, String FStyle, String* VStyle,int ID){	
		NameB = InName;			
		if(*NameB == true) { NameBLast = "true"; } else { NameBLast = "false"; }				
		Type = VTYPE_BOOL;	
		VarDesc = Desc;	
		FixStyle = FStyle;
		VarStyle = VStyle;		
		VarID = ID;		
	}	
	
	PageElement::~PageElement() {
		if(OptionList != nullptr) delete[] OptionList;
	}

//  Update the assigned variable with a new Value
    void PageElement::SetName(String InName){	
		switch (Type)    
		{ 	
			case VTYPE_STRING: 	
			    //Serial.print("STRING: "); Serial.println(InName); 
			 	*NameS = InName;	
				break;
			case VTYPE_DOUBLE: 
				//Serial.print("DOUBLE: "); Serial.println(InName); 
				*NameD = InName.toDouble();  	
				break;		
			case VTYPE_FLOAT: 
				//Serial.print("FLOAT: "); Serial.println(InName); 
				*NameF = InName.toFloat();  
				break;		
			case VTYPE_INT: 
				//Serial.print("INT: "); Serial.println(InName); 
				*NameI = InName.toInt();  
				break;		
			case VTYPE_BOOL: 
				//Serial.print("BOOL: "); Serial.println(InName); 
				if(InName == "true"){ *NameB = true; } else { *NameB = false; }						
				break;			
		}	
	} 	

// 	Retrieve the value of assigned variable
    String PageElement::GetName(){
		switch (Type)    
		{ 	
			case VTYPE_STRING: 	
				return *NameS;		
				break;
			case VTYPE_DOUBLE: 
				return String(*NameD);			
				break;		
			case VTYPE_FLOAT: 
				return String(*NameF);
				break;		
			case VTYPE_INT: 
				return String(*NameI);		
				break;		
			case VTYPE_BOOL:
				if(*NameB == true){ return "true"; } else { return "false"; }
				break;
		}
		return "";
	}

// 	Retrieve the Variable style of assigned variable
    const String& PageElement::GetVarStyle(){
		return *VarStyle;
	}
	
// 	Retrieve the previous value of assigned variable	
    String PageElement::GetNameLast(){

		switch (Type)    
		{ 	
			case VTYPE_STRING: 	
				return NameSLast;		
				break;
			case VTYPE_DOUBLE: 
				return String(NameDLast);			
				break;		
			case VTYPE_FLOAT: 
				return String(NameFLast);
				break;		
			case VTYPE_INT: 
				return String(NameILast);		
				break;		
			case VTYPE_BOOL:
				return NameBLast;
				break;
		}
		return "";
	}

//  Update the previous value of assigned variable
    void PageElement::UpdateLast(){

		switch (Type)    
		{ 	
			case VTYPE_STRING: 	
				NameSLast = *NameS;	
				break;
			case VTYPE_DOUBLE: 
				NameDLast = *NameD;			
				break;		
			case VTYPE_FLOAT: 
				NameFLast = *NameF;	 
				break;		
			case VTYPE_INT: 
				NameILast = *NameI;		
				break;		
			case VTYPE_BOOL: 
				if(*NameB == true) {NameBLast = "true";	} else { NameBLast = "false";}	
				break;			
		}		
	}	

// Force re-push: setting last to "" guarantees GetName() != GetNameLast() on next tick
    void PageElement::ClearLast(){
		switch (Type)
		{
			case VTYPE_STRING: 	NameSLast = "";       break;
			case VTYPE_DOUBLE: 	NameDLast = -1e300;   break;
			case VTYPE_FLOAT:  	NameFLast = -1e30f;   break;
			case VTYPE_INT:    	NameILast = INT_MIN;  break;
			case VTYPE_BOOL:   	NameBLast = "";       break;
		}
	}

// 	Retrieve the Description of assigned variable
//  Return true if the value has changed since last Update
    bool PageElement::HasChanged(){ return GetName() != GetNameLast(); }

	const String& PageElement::GetDescription(){
		return VarDesc;
	}

// 	Retrieve the Fixed Style
	const String& PageElement::GetFStyle(){
		return FixStyle;
	}

// 	Retrieve the Web ID of assigned variable
	String PageElement::GetID(const char* Type){
		String Temp = String(VarID);
		if(Type[0] != '\0') { Temp = Type; Temp += String(VarID); }
		return Temp;
	}

//  Change the default String to Select for the purpose of a dropdown on Web Page 
//  and defining the amount of options 	
    void PageElement::SetStringToSelect(int Options) {
		NameStringSelect = true;
		OptionCount = Options;
		OptionList = new SelectOptions[OptionCount];	 
	}	

//  Add a Option for the Select String	
    void PageElement::AddOption(String Option) { 
 		OptionList[OptionIndex].Option = Option;
 		if(OptionIndex < ( OptionCount - 1 )) OptionIndex++;
	}

// 	Retrieve the Select Option of assigned variable  	
	const String& PageElement::GetOption(int Idx) {
		return OptionList[Idx].Option;
	}

// 	Retrieve the Option Count of assigned variable  
	int PageElement::GetOptionCount() {
		return OptionCount;
	}

// 	Retrieve the TYPE of assigned variable	
	String PageElement::GetTypeStr(){
		switch (Type)    
		{ 	
			case VTYPE_STRING: 	
				if(NameStringSelect) { return "select"; } else { return "text"; }
				break;
			case VTYPE_DOUBLE: 
 				return "number";				
				break;		
			case VTYPE_FLOAT: 
 				return "number";	 
				break;		
			case VTYPE_INT: 
  				return "number";				
				break;		
			case VTYPE_BOOL:
				return "checkbox";
				break;
		}
		return "";
	}

// ─── PageElement Attribute Setters / Getters ──────────────────────────────────
    void PageElement::SetTextAlign(String align)                           { AttrTextAlign = align; }
    void PageElement::SetPlaceholder(String ph)                            { AttrPlaceholder = ph; }
    void PageElement::SetReadOnly(bool ro)                                  { AttrReadOnly = ro; }
    void PageElement::SetRange(float mn, float mx, float step)             { AttrMin = mn; AttrMax = mx; AttrStep = step; }
    const String& PageElement::GetAttrTextAlign()                          { return AttrTextAlign; }
    bool  PageElement::HasAttrMin()  { return !isnan(AttrMin); }
    bool  PageElement::HasAttrMax()  { return !isnan(AttrMax); }
    bool  PageElement::HasAttrStep() { return !isnan(AttrStep); }
    float PageElement::GetAttrMin()  { return AttrMin; }
    float PageElement::GetAttrMax()  { return AttrMax; }
    float PageElement::GetAttrStep() { return AttrStep; }
    const String& PageElement::GetAttrPlaceholder()                        { return AttrPlaceholder; }
    bool  PageElement::GetAttrReadOnly()                                   { return AttrReadOnly; }

// =============================================================================
// PageVar
// =============================================================================
// Lightweight wrapper that holds a pointer to one PageElement.  Pages allocate
// a PageVar array and populate it with PageElement pointers; MaintainWebService
// iterates the array to detect and broadcast variable changes.
// =============================================================================
	struct PageVar
	{
		PageElement* Var;
	};

// ─────────────────────────────────────────────────────────────────────────────
// beginHtmlStream  (inline helper — preferred for all full HTML page handlers)
// Opens an AsyncResponseStream with Connection: close so the TCP connection
// is released after delivery.  The caller prints each page fragment directly
// into the stream with response->print(...) then calls request->send(response).
//
// Returns nullptr if heap is too low or too fragmented to safely serve a page.
// In that case a 503 has already been sent; the caller must return immediately.
//
// Two conditions are checked — either alone can cause a hang or crash:
//
//   getFreeHeap() < 18000  — total free too low to build page content + keep
//                            WebSocket and lwIP buffers alive concurrently.
//
//   getMaxAllocHeap() < 10000 — heap too fragmented: allocating CBUF_RESERVE
//                            leaves < 3 KB as the largest remaining block, which
//                            starves concurrent WebSocket message allocations.
//                            Observed crash at m=9204 (hang); safe at m=10228.
// ─────────────────────────────────────────────────────────────────────────────
    #define HEAP_FREE_MIN    18000   // getFreeHeap()     — absolute floor
    #define HEAP_MAXBLK_MIN  10000   // getMaxAllocHeap() — fragmentation floor
    #define CBUF_RESERVE      5120   // AsyncResponseStream initial buffer (bytes)

// ─── Low-Memory Recovery ──────────────────────────────────────────────────────
// Each refused page increments LowMemFailCount; the first page that builds
// resets it. A short dip never reaches LOW_MEM_REBOOT_COUNT, but a stuck state
// (free 10–18 KB, or heap fragmented below a 10 KB block — neither trips the
// MIN_FREE_HEAP monitor) would leave the web UI dead until midnight. At the
// limit MaintainTimeCheck() (02_Time.cpp) logs and reboots from loop() — not
// from here, since this runs in the AsyncTCP task.
    #define LOW_MEM_REBOOT_COUNT  10   // Consecutive refused pages before reboot
    volatile uint32_t LowMemFailCount  = 0;   // Consecutive "Low memory" responses
    volatile uint32_t LowMemLastFree   = 0;   // getFreeHeap() at the last refusal
    volatile uint32_t LowMemLastMaxBlk = 0;   // getMaxAllocHeap() at the last refusal

inline AsyncResponseStream* beginHtmlStream(AsyncWebServerRequest* request) {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlk   = ESP.getMaxAllocHeap();
    if (freeHeap < HEAP_FREE_MIN || maxBlk < HEAP_MAXBLK_MIN) {
        LowMemLastFree   = freeHeap;
        LowMemLastMaxBlk = maxBlk;
        LowMemFailCount++;
        AsyncWebServerResponse* err = request->beginResponse(503, "text/html",
            "<html><head><meta http-equiv=\"refresh\" content=\"1\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"></head>"
            "<body style=\"font-family:sans-serif;text-align:center;padding:20px\">"
            "<h3>Low memory &mdash; retrying&hellip;</h3></body></html>");
        err->addHeader("Connection", "close");
        err->addHeader("Retry-After", "3");
        request->send(err);
        return nullptr;
    }
    LowMemFailCount = 0;
    AsyncResponseStream* response = request->beginResponseStream("text/html", CBUF_RESERVE);
    response->addHeader("Connection", "close");
    return response;
}

// ─────────────────────────────────────────────────────────────────────────────
// sendHtmlPage  (inline helper — kept for trivial one-liner responses only)
// Sends a 200 HTML response with Connection: close so the TCP connection is
// released immediately after delivery.  HTTP/1.1 defaults to keep-alive;
// without this, idle connections accumulate in the lwIP PCB pool until the
// pool is exhausted and new navigation requests can no longer get a socket.
// WebSocket upgrades (HTTP 101) are not affected — they set their own headers.
// ─────────────────────────────────────────────────────────────────────────────
inline void sendHtmlPage(AsyncWebServerRequest* request, const String& html) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html);
    response->addHeader("Connection", "close");
    request->send(response);
}

// ─────────────────────────────────────────────────────────────────────────────
// sendTextResponse  (inline helper — used by plain-text/API handlers)
// Mirrors sendHtmlPage but for text/plain responses.  Adds Connection: close
// so the TCP connection is released immediately, preventing PCB pool exhaustion.
// ─────────────────────────────────────────────────────────────────────────────
inline void sendTextResponse(AsyncWebServerRequest* request, const String& text) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", text);
    response->addHeader("Connection", "close");
    request->send(response);
}

 