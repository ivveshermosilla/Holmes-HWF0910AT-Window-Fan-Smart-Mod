# Changelog

## 0.3.19-clock-reconnect - 2026-10-08

- Request NTP after boot, LAN IP recovery, VAC recovery, AP client association,
  and PWA initial access or reconnection, with pending requests and retries.
- Request time synchronization at each Denver local 12:00 AM independently
  of manual synchronization, reconnects, and the normal SNTP periodic timer.
- Preserve a valid online clock when another browser opens the PWA; use browser
  time as an automatic fallback when the clock or LAN is unavailable.
- Expose request reasons, retries, completed NTP timestamps and daily requests
  in clock telemetry. Synchronization handlers do not issue motor commands.
- Update the PWA cache to `hwf0910at-pwa-v9` and add portable clock/PWA tests.

## 0.3.18-schedule-clock - 2026-10-08

- Restored autonomous time after reboot through SNTP, using Denver local time
  and its daylight-saving transitions for schedule boundaries.
- Kept browser time sync as a fallback even when a manually started fan is on.
- Made each active schedule window enforce its end time after manual web or
  physical-button changes cancel schedule-owned speed control.
- Preserved an already running manual mode when a schedule window begins; the
  schedule still turns it off at that window's end.
- Required confirmed VAC and a two-second safety interval before an active
  schedule resumes after boot or AC reconnection.
- Updated the PWA cache to `hwf0910at-pwa-v8`.
- Verified manual HIGH before a window, automatic scheduled start, manual LOW
  during a window, exact scheduled shutdowns, STA reconnection, and reboot
  into an active window. Restored the original seven-day schedule afterward.

## 0.3.17-wifi-recovery - 2026-09-15

- Added indefinite, non-blocking STA recovery with ESP auto-reconnect plus
  explicit retries at 5, 10, 20, and 30-second maximum intervals.
- Kept the recovery AP available while the household router is absent or still
  booting after a power outage.
- Added STA recovery telemetry and a manual `Reconnect LAN` command that does
  not alter fan, schedule, LED, or TRIAC state.
- Corrected the PWA indicator so browser access through the ESP recovery AP is
  no longer mislabeled as a successful LAN connection.
- Advanced the PWA cache to `hwf0910at-pwa-v7`.
- Verified a forced STA reconnection while a saved Daily schedule was driving
  the fan: TRIAC pulses continued, motor/MOC gates stayed active, and the static
  configured static LAN address returned.

## 0.3.16-zc-stability - 2026-09-11

- Fixed an unsigned timestamp race that could classify a live 120 Hz zero-cross
  signal as stale for one sample.
- Replaced loop-delay-based AC reconnection detection with a 250 ms pulse-gap
  marker captured by the zero-cross ISR. Short disturbances suspend firing but
  preserve the active mode and schedule.
- Moved the 24-hour power log and seven-day temperature history to on-demand API
  endpoints, reducing periodic status from about 11.8 KB to 4.6 KB.
- Reduced normal PWA polling from 1.5 to 3 seconds and refresh the diagnostic log
  only while System is visible.
- Deferred hourly temperature-history NVM writes while TRIAC output is active.
- Added ZC pulse age and sample elapsed-time telemetry and advanced the PWA cache
  to `hwf0910at-pwa-v6`.
- Verified 12 complete app-entry flows followed by 10 minutes with three status
  clients and one diagnostic client: 579 samples, 71,982 additional pulses, no
  invalid state or ZC sample, and no additional AC transition.

## 0.3.15-scheduler-power-log - 2026-09-08

- Fixed scheduled intermediate speeds: schedule-owned `CUSTOM` mode no longer
  depends on the manual speed-slider checkbox.
- Added a persistent 24-hour power/control event log with reset reasons, motor
  transitions, schedule/timer/button/web causes, AC/ZC interruptions, and TRIAC
  pulse-gap warnings.
- Deferred event-log NVM writes while TRIAC output is active, so opening or
  authenticating the PWA does not add flash latency to motor firing.
- Added a bilingual Power tracking card and full log overlay in System.
- Advanced the PWA shell cache to `hwf0910at-pwa-v5`.
- Verified 93% scheduled operation and twelve simulated new-device PWA sessions
  without a motor-state change or a new TRIAC timing warning.

## 0.3.14-dual-core-fire - 2026-09-01

- Moved zero-cross synchronized MOC timing to a dedicated high-priority task so
  PWA loading and login cannot interrupt motor firing.
- Added pulse-gap telemetry for live regression testing.
- Corrected a microsecond-wait rollover race found during live pulse testing.
- Pinned phase firing away from the application/OneWire core so motor timing,
  PWA serving and DS18B20 acquisition cannot preempt one another.
- Corrected front/rear project photography, hardware reuse, replacement wiring,
  new-button construction, BOM, and module-level connection documentation.

## 0.3.10-led-state - 2026-09-01

- Made the digital LED row reflect the physical function indicators.
- Kept OFF visually dark and represented custom speed with LOW plus HIGH.
- Completed Wi-Fi scan/provisioning and separate firmware/LittleFS OTA panels.

## 0.3.9-schedule-daily

- Added mutually exclusive Weekly and Daily schedules.
- Treated end times earlier than start as overnight intervals.
- Rejected equal start/end times as an unsafe ambiguous schedule.
- Added scheduled speed and global LED dimmer settings.

## 0.3.6-icons-ampm-dimmer through 0.3.8

- Added PWA icons, 12-hour mobile time wheels, global dimmer, app login memory,
  centered notifications, read-only initial synchronization, and mobile fixes.

## 0.3.0-ota-wifi through 0.3.5

- Added OTA partitions, AP+STA provisioning, static-IP configuration, day/English
  defaults, boot/VAC safety, schedules, timers, and responsive application views.

## 0.2.x

- Added complete web control, 85-100% custom speed, two-second startup boost,
  original physical-button state machine, and function-driven LEDs.

## 0.1.x

- Established zero-cross and temperature acquisition.
- Corrected the critical GPIO mapping to LED data on GPIO21 and MOC trigger on
  GPIO47; added stale-NVS rejection and safe button testing.
