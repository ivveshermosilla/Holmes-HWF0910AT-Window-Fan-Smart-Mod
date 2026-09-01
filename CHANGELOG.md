# Changelog

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
