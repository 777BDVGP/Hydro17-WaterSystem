// =============================================================================
// 11_Other.h — Manual Operations, Controller State & Button Declarations
// =============================================================================
// Declares the ButtonSelection class for manual operation buttons on the web
// page, and the NormalOperation / ControllerDisableEnable flags that govern
// whether the system runs automatic irrigation and valve sequences.
// =============================================================================

// ─── Manual Operation Button Index Constants ─────────────────────────────────
// Index values into ButtonSelection::Selection[].
	#define BUTTON_SELECTIONS              13   // Total number of manual operation buttons
	#define BUTTON_BOREHOLE                0    // Borehole pump relay
	#define BUTTON_VALVE_1                 1    // Irrigation solenoid valve 1
	#define BUTTON_VALVE_2                 2    // Irrigation solenoid valve 2
	#define BUTTON_VALVE_3                 3    // Irrigation solenoid valve 3
	#define BUTTON_VALVE_4                 4    // Irrigation solenoid valve 4
	#define BUTTON_VALVE_5                 5    // Irrigation solenoid valve 5
	#define BUTTON_VALVE_6                 6    // Irrigation solenoid valve 6
	#define BUTTON_SUPPLY_TANK_SOL         7    // Tank supply solenoid
	#define BUTTON_TANK_FILL_SOL           8    // Tank fill solenoid
	#define BUTTON_TANK_FILTER_PUMP_RELAY  9    // Tank filter pump relay
	#define BUTTON_POOL_LIGHT_RELAY        10   // Pool light relay
	#define BUTTON_POOL_PUMP_RELAY         11   // Pool filter pump relay
	#define BUTTON_PLUGS_SWITCH_RELAY      12   // General purpose plugs relay

// ─── Controller State Flags ───────────────────────────────────────────────────
// NormalOperation — false while the system is in manual or disabled mode.
//   InitFor_NOT_NormalOperation() clears it; InitFor_NormalOperation() restores it.
//   TIM_ENABLE_NORMAL_OPERATION auto-restores it after Params->ResetTimeOnContDisabled.
// ControllerDisableEnable — mirrors the I_SYSTEM_DISABLE_ENABLE hardware input;
//   also set via the web Disable/Enable buttons.
	bool NormalOperation          = true;   // true = automatic irrigation/tank logic runs
	bool ControllerDisableEnable  = true;   // true = controller is enabled
	bool CurrentSUSPEND_IRRI      = false;  // Snapshot of SUSPEND_IRRI used for change detection

// =============================================================================
// ButtonItem
// =============================================================================
// Represents one manual operation button on the web page.
//   ButtonDescription — label shown next to the button
//   OutputPin         — GPIO pin toggled when the button is pressed
// =============================================================================
class ButtonItem
{
public:
	String ButtonDescription = "Not Used"; // Display label for this button
 	int    OutputPin         = 0;          // GPIO output pin driven by this button
};

// =============================================================================
// ButtonSelection
// =============================================================================
// Holds all manual operation buttons.  Created once in CreateButton();
// the web page builder in 11_Other.cpp iterates over Selection[] to generate
// the manual operation form.
// =============================================================================
class ButtonSelection
{
public:
	ButtonSelection();   // Constructor — initialises button descriptions and pin assignments
	~ButtonSelection();  // Destructor

	ButtonItem Selection[BUTTON_SELECTIONS]; // All manual operation buttons
};

// ─── Global Instances ─────────────────────────────────────────────────────────
	ButtonSelection *ButtonObject;   // Global button manager (created in CreateButton())

// ─── Last Tank Fill Report Cache ─────────────────────────────────────────────
	String LastTankFillReport = ""; // Most recent tank fill log entry (shown on Home page)
