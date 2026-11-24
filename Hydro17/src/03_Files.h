// =============================================================================
// 03_Files.h — SPIFFS File System Utilities & Navigation IP Persistence
// =============================================================================
// Declares shared variables used by the file management pages (Files, Download,
// Upload, Delete), defines the SPIFFS file-name constants for all JSON config
// and log files, and declares the navigation IP load/save helpers.
// =============================================================================

// ─── General-Purpose String Buffers ──────────────────────────────────────────
// Used across multiple web page builders and file handlers.
	String OP01 = "";   // Primary URL/path scratch buffer
	String OP03 = "";   // Secondary display-text scratch buffer

	int    SetMess    = 0;   // Message code for append_FileOption() status display
	String SpesialStr = "";  // Message text injected by FA_MESS_RED / FA_MESS_GRN

// ─── SPIFFS File Handles ─────────────────────────────────────────────────────
	File f;       // Primary file handle used by file operations and reduce_file()
	File ftemp;   // Temporary file handle used when reducing log file size
	String line;  // Line buffer for readStringUntil() calls

// ─── Sorted File Directory Array ─────────────────────────────────────────────
// Populated by CreateDFileList(); holds up to File_array_LEN entries sorted
// alphabetically by extension+name for display in the Files/Download/Delete pages.
	#define File_array_LEN  50   // Maximum number of files listed in one directory scan
	#define Filename_LEN    32   // Maximum filename length (including null terminator)
	struct FileNameStruct
	{
		char FileName[Filename_LEN]; // File name as stored on SPIFFS
		char SortName[Filename_LEN]; // Sort key: extension first, then base name
		int  FileSize;               // File size in bytes
	};
	int StructIdx;                           // Active entry count in SortedFiles[]
	FileNameStruct SortedFiles[File_array_LEN]; // Sorted directory listing

// ─── File Sub-Page Selector Constants ────────────────────────────────────────
// Passed to FilePageSel() to highlight the currently active navigation tab.
 	#define PS_FILES_HOME       1   // HOME tab active
 	#define PS_FILES_OTHER      2   // OTHER tab active
 	#define PS_FILES            3   // FILES tab active
 	#define PS_FILES_DOWNLOAD   4   // Download tab active
 	#define PS_FILES_UPLOAD     5   // Upload tab active
 	#define PS_FILES_DELETE     6   // Delete tab active

// ─── About Page IP Display ────────────────────────────────────────────────────
// /About.json stores the connected IP for display on the About page.
// ClearAboutIP() and SaveAboutIP() in 15_About.cpp use this path constant.
	static constexpr const char* ABOUT_FILE_PATH = "/About.json";

// ─── Report Display Limit ─────────────────────────────────────────────────────
// Maximum number of log lines shown on the report web pages.
// Keeping this small bounds the page String size and prevents heap exhaustion
// on large log files.  Older entries are skipped with a count shown in the page.
	#define REPORT_LINES  25

// ─── Log & Config File Name Constants ────────────────────────────────────────
// Centralised here so all modules reference the same path strings.
	String StrFormatComplete = F("/FormatComplete.txt"); // Sentinel file — absence triggers SPIFFS format
	String StrGeneralLog     = F("/GeneralLog.txt");     // General event log (trimmed daily)
	String StrIrrigationLog  = F("/IrrigationLog.txt");  // Irrigation event log (trimmed daily)
	String StrTankFillLog    = F("/TankFillingLog.txt");  // Tank fill event log (trimmed daily)

	String StrElectricValves = F("/WaterValves.json");   // Valve names & configurations
	String StrSuspendDays    = F("/SuspendDays.json");   // Irrigation suspension day settings
	String StrIrrigation     = F("/Irrigation.json");    // Irrigation schedule data
	String StrPoolFilFile    = F("/PoolFilter.json");    // Pool filter schedule data
	String StrPoolLightFile  = F("/PoolLight.json");     // Pool light schedule data

// ─── Protected files ─────────────────────────────────────────────────────────
// /WiFiConfig.json holds WiFi passwords (05_WiFi.cpp): never listed, viewed
// or downloaded by the FILES pages.  Accepts "name" or "/name".
	bool IsProtectedFile(String name) {
		if (name.startsWith("/")) name = name.substring(1);
		return name.equalsIgnoreCase("WiFiConfig.json") || name.equalsIgnoreCase("WifiSettings.json") ||
		       name.equalsIgnoreCase("WifiProfiles.json");
	}
