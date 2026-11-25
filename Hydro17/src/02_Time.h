// =============================================================================
// 02_Time.h — Software Timers, NTP Configuration & Date/Time Variables
// =============================================================================
// Declares the TimerStruct type and the global Timer[] array used by all
// modules for non-blocking timing.  Also defines the NTP server, weekday
// lookup tables, and the shared DateTime / WeekDay display variables.
// =============================================================================

// ─── NTP Server ───────────────────────────────────────────────────────────────
	const char *ntpServer = "za.pool.ntp.org"; // South African NTP pool
	#define NTP_SYNC_WAIT_MS  10000            // SetupTime(): max wait for first NTP reply

// ─── Software Timer Infrastructure ───────────────────────────────────────────
// Each TimerStruct entry is a one-shot countdown timer.
//   Status    : true = counting down; false = expired / inactive
//   Units     : numeric amount (e.g. 600)
//   TimerUnits: time base — MS (1), SEC (1000), or MIN (60000)
//   ExpireTime: millis() value at which the timer expires
	struct TimerStruct
	{
		bool           Status;        // true = counting down; false = expired
		unsigned int   Units;         // Timer amount in TimerUnits
		unsigned int   TimerUnits;    // Time base: MS / SEC / MIN
		unsigned long  ExpireTime;    // Absolute millis() expiry timestamp
	};

	#define MS               1        // 1 ms time base
	#define SEC              1000     // 1 second time base
	#define MIN              60000    // 1 minute time base

// ─── Timer Index Constants ────────────────────────────────────────────────────
	#define TIM_LED_FLASHING_SPEED      0   // LED heartbeat toggle interval
	#define TIM_UPDATE_SYSTEM_TIME      1   // Web page clock refresh interval
	#define TIM_FREE_MEMEORY            2   // Stack/heap monitor interval
	#define TIM_ASP_RESET               3   // Hold time to trigger WiFi credential reset
	#define TIM_TANK_ON_DELAY           4   // Delay before tank filter pump starts
	#define TIM_TANK_SENSOR             5   // Tank level sensor averaging interval
	#define TIM_PRESSURE_SENSOR         6   // Filter pressure sensor averaging interval (300 ms)
	#define TIM_RESET_OPTION_MESS       7   // Duration to display option status message
	#define TIM_ENABLE_NORMAL_OPERATION 8   // Auto-restore to normal operation timeout
	#define TIM_DEBUG                   9   // Debug output interval

	#define TIM_TOTAL                  10   // Total number of software timers

// ─── Timer Array Initialisation ───────────────────────────────────────────────
// All timers start inactive (Status = false).
// Columns: Status    Units    TimerUnits   ExpireTime
	TimerStruct Timer[TIM_TOTAL] =
	{
		{ false,  600,    MS,   0 },   // TIM_LED_FLASHING_SPEED      — 600 ms LED heartbeat
		{ false,    1,   SEC,   0 },   // TIM_UPDATE_SYSTEM_TIME      — 1 s clock refresh
		{ false,   15,   SEC,   0 },   // TIM_FREE_MEMEORY            — 15 s memory monitor
		{ false,    5,   SEC,   0 },   // TIM_ASP_RESET               — 5 s button hold reset
		{ false,   10,   MIN,   0 },   // TIM_TANK_ON_DELAY           — 10 min pump start delay
		{ false,  500,    MS,   0 },   // TIM_TANK_SENSOR             — 500 ms sensor average
		{ false,  300,    MS,   0 },   // TIM_PRESSURE_SENSOR         — 300 ms filter pressure average
		{ false,    2,   SEC,   0 },   // TIM_RESET_OPTION_MESS       — 2 s message display
		{ false,   60,   MIN,   0 },   // TIM_ENABLE_NORMAL_OPERATION — 60 min auto-restore
		{ false,    5,   SEC,   0 },   // TIM_DEBUG                   — 5 s debug output
	};

// ─── Recovery Watchdog Thresholds ────────────────────────────────────────────
// MIN_FREE_HEAP        — bytes of free heap below which a reboot is forced.
// WIFI_LOST_REBOOT_SEC — seconds WiFi stays disconnected before reboot.
	#define MIN_FREE_HEAP         10000  // Reboot if free heap drops below 10 KB
	#define WIFI_LOST_REBOOT_SEC  60     // Reboot if WiFi lost for 60 s

// ─── Date/Time Display Variables ─────────────────────────────────────────────
	const char *YR_MONTH[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	String DateTime = "";       // Formatted display string: "DD Mon YYYY HH:MM:SS Day"

	const char *WEEK_DAY[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

	int MonthDay = -1;          // Last day-of-month processed; triggers midnight tasks on change

// ─── Month Length Table ───────────────────────────────────────────────────────
// Used by MonthDaysValue() for date arithmetic (February handled separately for leap years).
	const int MonthLen[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// ─── Weekday Counter ──────────────────────────────────────────────────────────
// Ref anchor: 2023-11-07 = Wednesday (Weekday = 3).
// DetermineWeekday() increments from the anchor to reach today's day number.
// 1 = Sunday ... 7 = Saturday.
	byte WeekDay = 4; // Initialised to Wednesday (2020-07-01 anchor); updated at midnight

// ─── Midnight Reboot Flag ─────────────────────────────────────────────────────
// Armed by MaintainMidnightReboot() outside the reboot minute; cleared on every
// power-on/reboot because it lives in RAM.  The reboot (SETTINGS time, default
// 00:05) fires only when the flag is true: exactly one scheduled reboot per night.
	bool MidnightRebootFlag = false;
