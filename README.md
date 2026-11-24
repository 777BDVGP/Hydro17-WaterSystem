# Hydro17 Water System — borehole, tank, irrigation & pool controller

A home water controller on a **WEMOS LOLIN S2 Mini** (ESP32-S2): it fills a water
tank from a borehole, runs up to **9 irrigation zones** on **6 solenoid valves**,
drives the tank filter pump, and switches the **pool filter pump**, **pool light**
and a **plugs relay** on schedules. It can skip watering after rain by asking a
[Rain Gauge](https://777bdvgp.github.io/RainGauge/). Everything is a web page on
your own WiFi — no cloud, no app.

**Install / update page:** https://777bdvgp.github.io/Hydro17-WaterSystem/

**Contents:** [Features](#features) · [Hardware](#hardware) · [Pin map](#pin-map) ·
[Installing the firmware](#installing-the-firmware) · [First start: WiFi](#first-start-wifi) ·
[Rain gauge](#rain-gauge) · [Updating](#updating) ·
[Building the firmware yourself](#building-the-firmware-yourself) · [Licence](#licence)

> **Who this is for:** someone with hands-on experience in electronics and
> irrigation plumbing. It is not a kit manual. The guide grows over time —
> parts marked **(to be completed)** get more detail and photos later.

## Features

- **HOME** — live status: tank level, float switches, borehole pump, valves,
  active irrigation zone, pool pump and light, maintenance due dates.
- **Irrigation** — 9 zones, each with its own valve configuration, start date /
  time, duration, every-N-days frequency, water source (tank or borehole),
  per-weekday suspend, manual or scheduled mode; a 7-day suspend calendar.
- **Rain gauge** (optional) — asks a Rain Gauge on the same network every hour
  whether to irrigate today; zones set to "Respect Rainfall" are skipped when it
  says no. Gauge not reachable for 3 hours = irrigate as normal.
- **Tank** — fill by float switch or level sensor, any time or inside a time
  window, start / stop levels, filter pump delay after filling.
- **Pool** — filter pump 7-day schedule with manual override, pool light on / off.
- **Valves** — names for the valves and 9 valve configurations.
- **Manual operation** — switch the controller off (all automatic logic stops),
  with automatic return to normal after a set time.
- **NTFY** — free push alerts on your phone ([ntfy.sh](https://ntfy.sh)), e.g.
  maintenance due dates.
- **Reports** — general, irrigation and tank-filling logs.
- **WiFi** — saved networks list (add / delete, passwords never shown), joins the
  strongest network in range and the **strongest access point** when several
  share one WiFi name (mesh / multi-AP). Setup hotspot **Hydro17-Setup** for a new
  board or when no network can be reached.
- **ABOUT** — **firmware update from GitHub** (CHECK / INSTALL) or from an uploaded `.bin`.
- **Safety** — software watchdog, gateway keepalive ping (reboots after 5 min
  without the router), low-memory guard, optional nightly reboot.

## Hardware

- WEMOS LOLIN S2 Mini (ESP32-S2, 4 MB flash, 2 MB PSRAM).
- Main PCB: Eagle files in [`PCB/`](PCB) (`WEMOS_ESP32_2S_V5.sch` / `.brd`).
  Gerbers **(to be completed)**.
- Float / relay board: `ManualFloatRelaysV2.sch` / `.brd` and its Gerber ZIP
  [`ManualFloatRelaysV2_2024-05-08.zip`](PCB/ManualFloatRelaysV2_2024-05-08.zip).
- Relay / driver outputs for the solenoid valves, borehole pump, tank filter
  pump, pool pump, pool light and plugs; inputs for float switches and the
  panel buttons / switches; analogue tank level sensor.
- Bill of materials, wiring and photos **(to be completed)**.

## Pin map

| Output | GPIO | | Input | GPIO |
|---|---|---|---|---|
| Tank supply solenoid | 4 | | Tank level sensor (ADC) | 1 |
| Irrigation valve 1 | 5 | | Pool filter sensor (ADC) | 2 |
| Irrigation valve 2 | 6 | | Tank filter sensor (ADC) | 3 |
| Irrigation valve 3 | 7 | | Tank filter on / off button | 33 |
| Irrigation valve 4 | 8 | | Controller disable / enable button | 34 |
| Irrigation valve 5 | 9 | | Pool filter switch | 35 |
| Irrigation valve 6 | 10 | | Pool light switch | 36 |
| Borehole pump relay | 11 | | Plugs on / off switch | 37 |
| Controller disabled LED | 12 | | Tank high float switch | 38 |
| Tank low LED | 13 | | Tank low float switch | 39 |
| Tank high LED | 14 | | BOOT button (WiFi setup) | 0 |
| Pool light relay | 16 | | | |
| Pool filter pump relay | 17 | | | |
| Plugs relay | 18 | | | |
| Tank filter pump relay | 21 | | | |
| Tank fill solenoid | 40 | | | |

## Installing the firmware

No PlatformIO needed: open **https://777bdvgp.github.io/Hydro17-WaterSystem/** in
Chrome or Edge on a PC, plug the S2 Mini in with a USB **data** cable and press
**CONNECT**. Choose **Erase** for a new board, then **Install**. If no port shows
up, hold **0**, tap **RST**, release **0** and try again (tap **RST** once more
after the install).

## First start: WiFi

1. A new controller opens its own hotspot **Hydro17-Setup** (the LED blinks fast).
2. Connect a phone to it — the setup page opens by itself (otherwise browse to `http://192.168.4.1/`).
3. Pick your network, type the password and give it a **fixed IP**, press **SAVE & RESTART**.
4. Put the phone back on your WiFi and open the fixed IP (or `http://hydro17.local/`).

More networks: **SETTINGS → WiFi Networks**. Hold **BOOT** or the **CONTROLLER**
button while powering up to open the setup hotspot again. If no saved network can
be reached for about 4 minutes, the hotspot opens by itself for 5 minutes, then
the controller restarts and tries again.

## Rain gauge

On **SETTINGS** tick **Use Rain Gauge** and enter the gauge's IP (e.g.
`192.168.1.124`). The controller asks `http://<gauge>/api/irrigate` every hour (and
right after saving); the badge under the settings shows **IRRIGATE** (green) or
**NO IRRIGATION TODAY** (red). Only zones with **Respect Rainfall = Yes** are
skipped.

## Updating

On the **ABOUT** page: **CHECK FOR UPDATE** compares the version with the one on the
install page; **INSTALL FROM GITHUB** downloads and installs it (not while a zone
irrigates or the borehole pump runs). Settings and logs are kept; a failed
download leaves the old firmware running. Without internet at the controller:
download [`hydro17.bin`](docs/firmware/hydro17.bin) and use **UPLOAD & INSTALL**.

## Building the firmware yourself

[PlatformIO](https://platformio.org/) project in [`Hydro17/`](Hydro17):

```bash
cd Hydro17
pio run -e usb -t upload                                   # first flash over USB
pio run -e ota -t upload --upload-port <controller ip>     # later over WiFi
```

`main.cpp` is the single translation unit — it `#include`s every module, so every
cross-file function needs a declaration in `src/00_ForwardDecl.h`.

Optional built-in WiFi networks: copy `src/wifi_secrets.example.h` to
`src/wifi_secrets.h` (git-ignored). Builds with that file are named `_PRIVATE` —
do not share them, your passwords are inside.

## Licence

[CC BY-NC 4.0](LICENSE) — free for personal use, not for sale. No warranty: this
controls pumps, valves and pool equipment, and they run on mains power — have
mains wiring done or checked by a qualified person.

Copyright (c) 2026 Bennie Vermeulen
