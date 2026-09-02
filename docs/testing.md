# Testing And Validation

## Bench Stages

1. Power-off continuity and component identification.
2. Isolated 5 V bring-up with mains motor output disabled.
3. LED and DS18B20 tests after correcting physical wiring.
4. Zero-cross frequency and four H11 activity checks at 120 VAC.
5. MOC/TRIAC firing with both original sliders in known positions.
6. HIGH/LOW/custom speed, two-second boost, OFF, thermostat, and button loop.
7. Timer and weekly/daily same-day and overnight schedule behavior.
8. PWA login, mobile layout, second-device read-only synchronization, and PWA assets.
9. Firmware and LittleFS OTA upload followed by automatic reboot and status check.

## Current Evidence

- Reference version: `0.3.14-dual-core-fire`.
- Zero-cross telemetry near 120 edge events per second on 60 Hz mains.
- DS18B20 live values with no current read-error accumulation.
- MOC remains disarmed while OFF; commanded pulse count increments only when armed.
- Four slider optocoupler channels expose live activity and Intake/Exhaust labels.
- Desktop and mobile screenshots were captured directly from the installed PWA.
- The operating sketch compiled at 1,077,365 bytes (82% of the app partition)
  with 50,380 bytes (15%) of global RAM. The sanitized public sketch compiled at
  1,077,233 bytes with the same global RAM use.
- Twelve complete PWA downloads plus twelve new-device logins completed with no
  HTTP failures while HIGH remained active.
- During that test `firePulseCount` reached 7,714, zero-cross stayed at 120.0 Hz,
  and the maximum fire gap was 8,437 us, below a missed 60 Hz half-cycle.
- DS18B20 recorded zero read, reset, and CRC errors through 67 successful resets
  during the same motor/network load test.

## Limitations

No current transformer, tachometer, or TRIAC-output voltage feedback is installed.
Therefore software cannot independently prove blade rotation, delivered RMS
voltage, or TRIAC conduction. Device Health reports prerequisites and command
activity, not a closed-loop motor measurement.

Still pending: extended thermal soak, quantified airflow/noise, calibrated LED
color transfer, electrical waveform captures, and a final operating video.
