// =============================================================================
// 05_ThisSystem.h — GPIO Definitions, I/O Structures & Page Selector Constants
// =============================================================================
// Defines every GPIO pin assignment for the Hydro17 Water System hardware,
// declares the debounced input and output port arrays, and defines the
// page-selector constants used by the navigation menu builders.
// =============================================================================

// ─── GPIO Pin Number Definitions ─────────────────────────────────────────────
// All ESP32-S2 GPIOs used by this board, named by their assigned function.
	#if defined(ARDUINO_ARCH_ESP8266)
		// No ESP8266 assignments for this board
	#elif defined(ARDUINO_ARCH_ESP32)

	// ── Sensor Input Pins ─────────────────────────────────────────────────────
	#define GPIO_01  1   // S_TANK_LEVEL_SENSE  — Analog tank level sensor input
	#define GPIO_02  2   // S_POOL_FILTER_SENSE — Analog Pool filter sensor input 
	#define GPIO_03  3   // S_TANK_FILTER_SENSE — Analog Tank filter Sensor input

	// ── Output Solenoid & Relay Pins ─────────────────────────────────────────
	#define GPIO_04  4   // O_TANK_SUPPLY_SOLENOID    — Tank supply solenoid valve
	#define GPIO_05  5   // O_VALVE_SOL_1             — Irrigation solenoid valve 1 
	#define GPIO_06  6   // O_VALVE_SOL_2             — Irrigation solenoid valve 2
	#define GPIO_07  7   // O_VALVE_SOL_3             — Irrigation solenoid valve 3
	#define GPIO_08  8   // O_VALVE_SOL_4             — Irrigation solenoid valve 4
	#define GPIO_09  9   // O_VALVE_SOL_5             — Irrigation solenoid valve 5
	#define GPIO_10  10  // O_VALVE_SOL_6             — Irrigation solenoid valve 6
	#define GPIO_11  11  // O_BOREHOLE                — Borehole pump relay
	#define GPIO_12  12  // O_SYSTEM_DISA_ENA_LED     — System disable/enable indicator LED
	#define GPIO_13  13  // O_TANK_LOW_LED            — Tank low level indicator LED
	#define GPIO_14  14  // O_TANK_HIGH_LED           — Tank high level indicator LED
	#define GPIO_16  16  // O_POOL_LIGHT_RELAY        — Pool light relay
	#define GPIO_17  17  // O_POOL_PUMP_RELAY         — Pool filter pump relay
	#define GPIO_18  18  // O_PLUGS_SWITCH_RELAY      — General purpose plugs relay
	#define GPIO_21  21  // O_TANK_FILTER_PUMP_RELAY  — Tank filter pump relay
	#define GPIO_40  40  // O_TANK_FILL_SOLENOID      — Tank fill solenoid valve

	// ── Digital Input Pins ────────────────────────────────────────────────────
	#define GPIO_33  33  // I_TANK_FILTER_ON_OFF      — Tank filter manual on/off button
	#define GPIO_34  34  // I_SYSTEM_DISABLE_ENABLE   — System controller disable/enable button
	#define GPIO_35  35  // I_POOL_FILTER             — Pool filter manual on/off switch
	#define GPIO_36  36  // I_POOL_LIGHT              — Pool light manual on/off switch
	#define GPIO_37  37  // I_PLUGS_ON_OFF            — Plugs manual on/off switch
	#define GPIO_38  38  // I_FLOAT_HIGH              — Tank high-level float switch
	#define GPIO_39  39  // I_FLOAT_LOW               — Tank low-level float switch

	// ── Analog Input ──────────────────────────────────────────────────────────
	#define INPIDX_CURRENT  03    // ADC channel for current transformer (EmonLib)

	#endif  // ARDUINO_ARCH_ESP32

// ─── Output Port Array Index Constants ───────────────────────────────────────
// Index values into OutPort[] — use these names rather than raw numbers.
    #define O_TANK_SUPPLY_SOLENOID      0     // Pin GPIO_04
	#define O_VALVE_SOL_1               1     // Pin GPIO_05
    #define O_VALVE_SOL_2               2     // Pin GPIO_06
	#define O_VALVE_SOL_3               3     // Pin GPIO_07
    #define O_VALVE_SOL_4               4     // Pin GPIO_08
	#define O_VALVE_SOL_5               5     // Pin GPIO_09
    #define O_VALVE_SOL_6               6     // Pin GPIO_10
	#define O_BOREHOLE                  7     // Pin GPIO_11
    #define O_SYSTEM_DISA_ENA_LED       8     // Pin GPIO_12
	#define O_TANK_LOW_LED              9     // Pin GPIO_13
    #define O_TANK_HIGH_LED             10    // Pin GPIO_14
	#define O_POOL_LIGHT_RELAY          11    // Pin GPIO_16
    #define O_POOL_PUMP_RELAY           12    // Pin GPIO_17
	#define O_PLUGS_SWITCH_RELAY        13    // Pin GPIO_18
    #define O_TANK_FILTER_PUMP_RELAY    14    // Pin GPIO_21
	#define O_TANK_FILL_SOLENOID        15    // Pin GPIO_40

// ─── Input Port Array Index Constants ────────────────────────────────────────
// Index values into InPort[] — use these names rather than raw numbers.
    #define I_TANK_FILTER_ON_OFF     0  // Pin GPIO_33
    #define I_SYSTEM_DISABLE_ENABLE  1  // Pin GPIO_34
    #define I_POOL_FILTER            2  // Pin GPIO_35
    #define I_POOL_LIGHT             3  // Pin GPIO_36
    #define I_PLUGS_ON_OFF           4  // Pin GPIO_37
    #define I_FLOAT_HIGH             5  // Pin GPIO_38 — tank high-level float switch
    #define I_FLOAT_LOW              6  // Pin GPIO_39 — tank low-level float switch

// ─── Input Debounce Configuration ────────────────────────────────────────────
// All digital inputs are software-debounced.  A pin must hold its new state
// for DEBOUNCE_STEADY_MS before InPort[].Input is updated.
	unsigned long DebouceTimeRead;
	#define DEBOUNCE_STEADY_MS  50    // 50 ms steady-state required to accept a new level

	struct DebounceStruct
	{
		const int PinIO;  // Physical GPIO pin number
		bool MemoryH;     // Rising-edge memory flag
		bool MemoryL;     // Falling-edge memory flag
		bool Input;       // Debounced logical state (true = HIGH)
		unsigned long Time; // Timestamp of first edge seen (ms)
	};

	#define InputPorts  7
	// Columns: PinIO      MemoryH  MemoryL  Input   Time
	DebounceStruct  InPort[InputPorts] =
    {
		{GPIO_33,   false,   false,   false,   0 }, // I_TANK_FILTER_ON_OFF
		{GPIO_34,   false,   false,   false,   0 }, // I_SYSTEM_DISABLE_ENABLE
		{GPIO_35,   false,   false,   false,   0 }, // I_POOL_FILTER
		{GPIO_36,   false,   false,   false,   0 }, // I_POOL_LIGHT
		{GPIO_37,   false,   false,   false,   0 }, // I_PLUGS_ON_OFF
		{GPIO_38,   false,   false,   false,   0 }, // I_FLOAT_HIGH
		{GPIO_39,   false,   false,   false,   0 }, // I_FLOAT_LOW
	};

// ─── Output Port Array ────────────────────────────────────────────────────────
// Valve outputs MUST occupy the first SOL_OUTPUT_COUNT entries so that the
// valve switching logic can iterate over them by index.
	struct OutPortStruct
	{
		int    Port;    // Physical GPIO pin number
		String Desc;    // Human-readable description (used in reports and Serial output)
		bool   OUTStat; // Current commanded output state (true = HIGH / active)
	};

	#define OutPorts  16
	// Columns: Port        Desc                       OUTStat
    OutPortStruct OutPort[OutPorts] =
    {
      { GPIO_04, "TANK_SUPPLY_SOLENOID  ", false }, // O_TANK_SUPPLY_SOLENOID
      { GPIO_05, "VALVE_SOL_1           ", false }, // O_VALVE_SOL_1
      { GPIO_06, "VALVE_SOL_2           ", false }, // O_VALVE_SOL_2
      { GPIO_07, "VALVE_SOL_3           ", false }, // O_VALVE_SOL_3
      { GPIO_08, "VALVE_SOL_4           ", false }, // O_VALVE_SOL_4
      { GPIO_09, "VALVE_SOL_5           ", false }, // O_VALVE_SOL_5
      { GPIO_10, "VALVE_SOL_6           ", false }, // O_VALVE_SOL_6
      { GPIO_11, "BOREHOLE              ", false }, // O_BOREHOLE
      { GPIO_12, "SYSTEM_DISA_ENA_LED   ", false }, // O_SYSTEM_DISA_ENA_LED
      { GPIO_13, "TANK_LOW_LED          ", false }, // O_TANK_LOW_LED
      { GPIO_14, "TANK_HIGH_LED         ", false }, // O_TANK_HIGH_LED
      { GPIO_16, "POOL_LIGHT_RELAY      ", false }, // O_POOL_LIGHT_RELAY
      { GPIO_17, "POOL_PUMP_RELAY	    ", false }, // O_POOL_PUMP_RELAY
      { GPIO_18, "PLUGS_SWITCH_RELAY    ", false }, // O_PLUGS_SWITCH_RELAY
      { GPIO_21, "TANK_FILTER_PUMP_RELAY", false }, // O_TANK_FILTER_PUMP_RELAY
      { GPIO_40, "TANK_FILL_SOLENOID    ", false }, // O_TANK_FILL_SOLENOID
    };

// ─── Main Page Selector Constants ────────────────────────────────────────────
// Passed to appendMainPageSel() to highlight the active navigation tab.
 	#define PS_HOME              1
	#define PS_REPORTS           2
 	#define PS_IRRIGATION        3
	#define PS_TANK              4
	#define PS_POOL              5
	#define PS_POOL_LIGHT        10
	#define PS_PLUGS             6
 	#define PS_OTHER             7
	#define PS_SETTINGS          8
	#define PS_ABOUT             9
	#define PS_NTFY              15

	#define PS_IRRIGATION_MANUAL     11   // Manual irrigation operations sub-page
	#define PS_IRRIGATION_SUSPEND    12   // Irrigation suspension management sub-page
	#define PS_IRRIGATION_VALVES     13   // Valve names configuration sub-page
 	#define PS_IRRIGATION_CONFIG     14   // Valve combinations configuration sub-page

	#define PS_REPORT_GEN            16   // General log report page  (/reportgen)
	#define PS_REPORT_PUMP           17   // Irrigation log report page (/reportpump)
	#define PS_REPORT_FILL           18   // Tank fill log report page (/reportfill)

// ─── Web Table Builder Constants ─────────────────────────────────────────────
// Passed to WT_Column() to describe cell type and column position.
	#define WT_LABEL    0   // Cell contains a label (read-only display)
	#define WT_INPUT    1   // Cell contains an input element (editable)
	#define WT_FIRST    2   // First column in a new row
	#define WT_NEXT     3   // Middle column in a row
	#define WT_MORE     5   // Additional column (same row, no special position)
	#define WT_LAST     4   // Last column in a row (closes the <tr>)
