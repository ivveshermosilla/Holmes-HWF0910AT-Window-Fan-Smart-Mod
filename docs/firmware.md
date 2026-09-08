# Firmware

The Arduino sketch is in
[`firmware/Holmes_HWF0910AT_Smart_Mod/`](../firmware/Holmes_HWF0910AT_Smart_Mod/).
The `data/` directory is the LittleFS image served by the ESP32-S3.

## Safety State Machine

At boot the MOC output is forced low and the mode is `OFF_LOOP`. When zero-cross
activity first establishes that VAC is present, the controller again forces OFF;
restored Wi-Fi, NVM settings, or a browser connection cannot start the motors.

Short button presses follow:

```text
OFF -> HIGH -> LOW -> HIGH 60 -> HIGH 65 -> HIGH 70 -> HIGH 75 -> HIGH 80
    -> LOW 60 -> LOW 65 -> LOW 70 -> LOW 75 -> LOW 80 -> OFF -> ...
```

A long press of approximately 2.5 seconds from an active original mode enters a
volatile `OFF_MEMORY`; the next short press restores that mode. Removing power
clears the memory. A short press from a custom web speed first returns to OFF.

Every stopped-to-running transition applies 100% for two seconds, then moves to
the requested phase delay. Adjusting an already-running fan does not repeat the
boost.

Zero-cross events wake a dedicated high-priority FreeRTOS task that owns the
short GPIO47/MOC pulse. HTTP serving remains in the application loop, but cannot
delay the phase-fire task. Status telemetry exposes `fireTaskReady`, pulse count,
last pulse gap, and maximum pulse gap so this separation can be regression-tested
while another device loads or logs into the PWA.

## Thermostat And Schedule

Thermostat modes stop at or below the target and resume above it with configured
hysteresis. Weekly entries share one interval among selected start days. Daily
entries retain a separate interval per selected day. An end earlier than start
belongs to the following day; equal times disable that interval.

Schedules and their speed/dimmer values persist in ESP32 Preferences NVM.
Temperature history stores one hourly sample for seven days in NVM.

Scheduled intermediate speeds use `SPEED_CUSTOM` under schedule ownership. This
authority is independent of the manual custom-slider checkbox: the checkbox
continues to govern direct web control, while an active schedule can apply its
own saved 85-100% value without changing that preference.

## Persistent Power Tracking

Firmware keeps a bounded ring of up to 64 events and exposes the newest entries
under `powerLog` in `/api/status`. Once local time is synchronized, entries older
than 24 hours are removed. Events include ESP reset reason, AC detection/loss,
motor ON/OFF and its requesting subsystem, app session checks, MOC output
blocking/restoration, and TRIAC firing gaps above 12,500 microseconds.

Event capture is immediate in RAM. NVM persistence is deferred whenever the
motor request or TRIAC fire task is active, preventing a diagnostic write from
adding flash latency to phase firing. The next OFF transition commits the full
pending log. This records software decisions and pulse timing; without a
tachometer or isolated current sensor it cannot prove physical blade rotation.

## Build

Reference Arduino-ESP32 target options:

```text
esp32:esp32:esp32s3
FlashSize=4M, PartitionScheme=default, FlashMode=dio, CPUFreq=240
PSRAM=disabled
```

Compile with Arduino IDE/CLI using the ESP32 core and the Adafruit NeoPixel
library. Build LittleFS separately from `data/`; for the current 4 MB default
partition layout its image size is `0x160000`. Confirm the partition table from
the compiled build rather than assuming an offset on different boards.

## OTA

System settings accept two distinct Arduino binary artifacts:

- Firmware: the compiled sketch `.bin` sent to the firmware endpoint.
- UI: the generated LittleFS `.bin` sent to the LittleFS endpoint.

The browser displays upload progress; the ESP validates the write and reboots
automatically after success. Keep power stable. Never upload a filesystem image
whose size or partition scheme differs from the installed firmware.

## First Commissioning

The public source defaults to AP `IvvesFan-Config` and example subnet
`192.168.1.x`; no household SSID or password is embedded. Generic app and OTA
credentials are intentionally visible in source for first commissioning. Change
all credentials from a trusted connection before permanent installation.
