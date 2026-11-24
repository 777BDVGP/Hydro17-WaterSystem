// =============================================================================
// 22_Update.h — Firmware update from GitHub or from an uploaded .bin (ABOUT)
// =============================================================================
// Ported from Prima3_pomphuis / RainGauge_PIO 22_Update (2026-11-24).
// ABOUT page, "FIRMWARE UPDATE":
//
//   CHECK FOR UPDATE  reads FW_UPDATE_URL "version.json" from GitHub Pages:
//                     {"version":"2.0.0","bin":"firmware/hydro17.bin","notes":"..."}
//   INSTALL           downloads that .bin into the spare OTA slot and restarts.
//   UPLOAD .bin       a file chosen on a PC / phone (POST /update).
//
// Both ways write the NEW firmware into the spare app partition (2 x 1.5 MB,
// partitions_ota.csv); the controller only switches to it when the whole
// image arrived and verified.  A broken download / wrong file leaves the
// running firmware untouched.  Settings and logs (SPIFFS) are never touched.
// A file must be an ESP32-S2 image AND contain FW_MARKER, so firmware of
// another project is refused.
//
// Not while irrigating or while the borehole pump runs (the restart would
// cut the run short) — MaintainFwUpdate() refuses with a status message.
//
// HTTPS: only the roots in GH_ROOT_CERTS are trusted (certificate IS
// checked).  The clock must be set (NTP).  mbedTLS buffers go to PSRAM while
// downloading.
//
// The GitHub check / install run in loop() and BLOCK it (HTTPClient) — the
// one accepted exception to "no blocking network calls in loop()": started by
// hand, takes ~20-60 s.  The 1 s watchdog Ticker is paused meanwhile (B's
// watchdog time is only a few seconds; the TLS handshake can take longer).
// =============================================================================

	#define FW_UPDATE_URL   "https://777bdvgp.github.io/Hydro17-WaterSystem/"   // GitHub Pages of the public repo
	#define FW_MARKER       "#HYDRO17-WATERSYSTEM-S2-FIRMWARE"              // must be in every Hydro17 .bin ('#' once)

	String FwLatest   = "-";     // version on GitHub (after CHECK)
	String FwBinPath  = "";      // "firmware/hydro17.bin" from version.json
	String FwNotes    = "";
	String FwStatus   = "-";     // shown live on ABOUT
	volatile bool FwBusy = false;  // an update (GitHub or upload) is running
