// =============================================================================
// 15_About.h — About Page & OTA Update Declarations
// =============================================================================
// Declares the About page route handler, the OTA setup function, and the
// SetASP / SetReboot control handlers.
// OTA IP and terminal command are derived live from WiFi.localIP().
// Build date comes from compiler __DATE__/__TIME__; download date is persisted
// on SPIFFS so it survives power cycles.
// =============================================================================

#ifndef ABOUT_H
#define ABOUT_H

#include <ArduinoOTA.h>

// ─── OTA State ────────────────────────────────────────────────────────────────
extern bool OTAInProgress;       // true while an OTA upload is in progress
extern int  OTAPercentComplete;  // Upload progress 0–100 %

// ─── About.json IP Helpers ────────────────────────────────────────────────────
void ClearAboutIP();                                    // Remove ip/gw/sn/dns from About.json before ClearWifiSettings()
void SaveAboutIP(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns); // Persist connected IP into About.json

// ─── Function Declarations ────────────────────────────────────────────────────
void SetupOTA();                                        // Configure ArduinoOTA callbacks and call begin()
void handleAbout(AsyncWebServerRequest *request);       // GET /about — serves the About page
void handleSetASP(AsyncWebServerRequest *request);      // GET /setASP — erase WiFi creds + reboot
void handleSetReboot(AsyncWebServerRequest *request);   // GET /setReboot — soft reboot

#endif // ABOUT_H
