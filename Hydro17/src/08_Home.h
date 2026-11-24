// =============================================================================
// 08_Home.h — Home Page Status Strings, Attributes & Display Variables
// =============================================================================
// Declares the status string constants, colour attribute macros, and the live
// String/Att variable pairs that drive every row on the Home page.
// MaintainHomePageValues() updates these from system state; MapHomePage() maps
// them to the PageVar list that the WebSocket broadcaster checks for changes.
// =============================================================================

// ─── Instruction & Colour State ──────────────────────────────────────────────
	String ACTION_INST = "";                  // Instruction text displayed in the page header
	String COLOR       = COLOR_DEFAULT;       // Current header colour (overridden by SetMessColour())

// ─── Home Page Row Index Constants ───────────────────────────────────────────
// Each constant is an index into the HomeList PageVar array.
// The order here must match the order in MapHomePage().
 	#define HOME_CONTROLLERST       0   // Controller status (Active / Not Active)
 	#define HOME_BORHOLESTATU       1   // Borehole pump status
 	#define HOME_TANKSUPPIRRI       2   // Tank supply solenoid status
 	#define HOME_TANKFILLINGS       3   // Tank filling status
 	#define HOME_TANKFILTER         4   // Tank filter pump status
 	#define HOME_TANKFLOATLEV       5   // Tank float sensor level
 	#define HOME_TANKLEVEL          6   // Tank level percentage (from sensor)
	#define HOME_FILTER_PRESSURE    7   // Filter pressure in kPa
 	#define HOME_IRRIGATION         8   // Irrigation zone status
 	#define HOME_POOLFILTER         9   // Pool filter pump status
 	#define HOME_POOLFILTERMAN      10  // Pool filter mode (Scheduled / Manual)
 	#define HOME_POOLLIGHT          11  // Pool light status
 	#define HOME_POOLLIGHTMAN       12  // Pool light mode (Scheduled / Manual)
//	#define HOME_PLUGS              13  // Plugs relay status — reserved for future use
 	#define HOME_POOL_BACKWASH      13  // Next pool backwash due date
 	#define HOME_TANK_BACKWASH      14  // Next tank backwash due date
 	#define HOME_CLEAN_BH_FILTER    15  // Next borehole filter clean date
 	#define HOME_REP_BLUE_FILTER    16  // Next blue filter replacement date
 	#define HOME_UV_TUBE_REPLACE    17  // Next UV tube replacement date
 	#define HOME_CHLORINE_MAINT     18  // Next pool chlorine maintenance date

	#define PAGE_VAR_HOME           19  // Total PageVar entries in HomeList

// ─── Row Colour Attribute Macros ─────────────────────────────────────────────
// Applied to the *Att strings to highlight status rows.
 	#define ATT_RED     "color:white;background-color:red;padding:4px 10px;border-radius:6px"
	#define ATT_WHITE   "color:black;background-color:white"
	#define ATT_GREEN   "color:white;background-color:green;padding:4px 10px;border-radius:6px"
 	#define ATT_ORANGE  "color:black;background-color:orange;padding:4px 10px;border-radius:6px"
 	#define ATT_YELLOW  "color:black;background-color:yellow;padding:4px 10px;border-radius:6px"

// ─── Status String Constants ──────────────────────────────────────────────────
// Human-readable labels used in the live Str* variables below.
  	#define HOME_CONTROL_ACTIVE     "Active"
  	#define HOME_CONTROL_NACTIVE    "Not Active"

  	#define HOME_BOREHO_STAT_NOTB   "Off"
 	#define HOME_BOREHO_STAT_BUSY   "Busy Pumping"

  	#define HOME_TANKSUPPLY_NO      "Closed"
 	#define HOME_TANKSUPPLY_YES     "Open"

  	#define HOME_TANKFILL_NO_BUSY   "Switched Off"
 	#define HOME_TANKFILL_BUSY      "Busy Filling"

  	#define HOME_TANKFILTER_OFF     "Switched Off"
 	#define HOME_TANKFILON_PUMPON   "Busy to Pump & Filter"
 	#define HOME_TANKFILTER_ON      "Busy Filtering"

  	#define HOME_TANKFLOAT_HIGH     "Level is High"
 	#define HOME_TANKFLOAT_LOW      "Level is Low"
 	#define HOME_TANKFLOAT_LOW_TIM  "Low - Waiting Time"
 	#define HOME_TANKFLOAT_DIS      "Disabled"

  	#define HOME_IRRIGATION_OFF     "None"
  	#define HOME_IRRIGATION_SUSP    "Suspended"

  	#define HOME_POOLFILTER_OFF     "Switched Off"
  	#define HOME_POOLFILTER_ON      "Busy Filtering"

  	#define HOME_POOLFILTERMAN_OFF  "Scheduled"
  	#define HOME_POOLFILTERMAN_ON   "Manual"

  	#define HOME_POOLLIGHT_OFF      "Switched Off"
  	#define HOME_POOLLIGHT_ON       "Switched On"

  	#define HOME_POOLLIGHTMAN_OFF   "Scheduled"
  	#define HOME_POOLLIGHTMAN_ON    "Manual"

  	#define HOME_PLUGS_OFF          "Switched Off"
  	#define HOME_PLUGS_ON           "Switched On"


// ─── PageVar List Pointer ─────────────────────────────────────────────────────
	PageVar* HomeList;  // Allocated in MapHomePage(); browsed by MaintainHomePageValues()

// ─── Live Status Variables ────────────────────────────────────────────────────
// Each pair: Str* holds the display string; *Att holds the CSS colour attribute.
// Updated by MaintainHomePageValues() and pushed to the browser via WebSocket.

	String StrControllerStatus    = HOME_CONTROL_ACTIVE;    String StrControllerStatusAtt = ATT_GREEN;
	String StrBoreholeStatus      = HOME_BOREHO_STAT_NOTB;  String StrBoreholeStatusAtt   = ATT_GREEN;
	String StrTankSupplyIrri      = HOME_TANKSUPPLY_NO;     String StrTankSupplyIrriAtt   = ATT_GREEN;
	String StrTankFilling         = HOME_TANKFILL_NO_BUSY;  String StrTankFillingAtt      = ATT_GREEN;
	String StrTankFilter          = HOME_TANKFILTER_OFF;    String StrTankFilterAtt       = ATT_GREEN;
	String StrFilterPressure      = "0 kPa";                String StrFilterPressureAtt   = ATT_GREEN;
	String StrTankFloatSensor     = HOME_TANKFLOAT_HIGH;    String StrTankFloatSensorAtt  = ATT_GREEN;
	String StrTankLevel           = "85%";                  String StrTankLevelAtt        = ATT_GREEN;
	String StrIrrigate            = HOME_IRRIGATION_OFF;    String StrIrrigateAtt         = ATT_GREEN;
	String StrPoolFilter          = HOME_POOLFILTER_OFF;    String StrPoolFilterAtt       = ATT_GREEN;
	String StrPoolFilterMan       = HOME_POOLFILTERMAN_OFF; String StrPoolFilterManAtt    = ATT_GREEN;
	String StrPoolLight           = HOME_POOLLIGHT_OFF;     String StrPoolLightAtt        = ATT_GREEN;
	String StrPoolLightMan        = HOME_POOLLIGHTMAN_OFF;  String StrPoolLightManAtt     = ATT_GREEN;
	String StrPlugs               = HOME_PLUGS_OFF;         String StrPlugsAtt            = ATT_GREEN;

// ─── Maintenance Due Date Display Variables ───────────────────────────────────
// Each pair: Str* holds the next due date (YYYY-MM-DD); *Att holds the colour.
// Updated daily by MaintainDueDatesValues() — yellow when due soon, red when overdue.
	String StrNextPoolBack        = "YYYY-MM-DD";  String StrNextPoolBackAtt    = ATT_GREEN;
	String StrNextTankBack        = "YYYY-MM-DD";  String StrNextTankBackAtt    = ATT_GREEN;
	String StrNextBHFilter        = "YYYY-MM-DD";  String StrNextBHFilterAtt    = ATT_GREEN;
	String StrNextRepBlueFil      = "YYYY-MM-DD";  String StrNextRepBlueFilAtt  = ATT_GREEN;
	String StrUVTubeReplace       = "YYYY-MM-DD";  String StrUVTubeReplaceAtt   = ATT_GREEN;
	String StrChlorineMaint       = "YYYY-MM-DD";  String StrChlorineMainAtt    = ATT_GREEN;

// ─── Time Variables ───────────────────────────────────────────────────────────
	int InternetSec = 0;  // Current second from NTP (used for scheduling triggers)
	int InternetMin = 0;  // Current minute from NTP (used for scheduling triggers)

// ─── Days-Until-Due Counters ──────────────────────────────────────────────────
// Calculated daily by MaintainDueDatesValues(); used to colour the due-date rows.
	int DaysToBackwashPool;        // Days until next pool backwash
	int DaysToBackwashTank;        // Days until next tank backwash
	int DaysToCleanBHFilter;       // Days until next borehole filter clean
	int DaysToReplaceBlueFilter;   // Days until next blue filter replacement
	int DaysToReplaceUVTube;       // Days until next UV tube replacement
	int DaysToChlorine;            // Days until next pool chlorine maintenance
