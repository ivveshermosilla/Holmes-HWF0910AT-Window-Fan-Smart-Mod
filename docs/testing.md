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

- Reference version: `0.3.16-zc-stability`.
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
- A live schedule test selected `CUSTOM` at 93% with the manual custom checkbox
  false. Status reported mode 13, `motorRequested=true`, `fireAllowed=true`,
  schedule driving, 93%, and the scheduled 37% LED dimmer after the two-second
  start boost.
- Twelve simulated new-device flows each requested session, login, application,
  config, and status while that 93% schedule was active. TRIAC pulses increased
  from 868 to 2447, the maximum gap remained 9487 us, and no output-gap or
  output-blocked event appeared.
- Restoring the saved Daily schedule produced matching persistent ON/OFF events
  at 93%, returned the controller to OFF, and preserved the original seven-day
  schedule values.
- The 0.3.15 LittleFS image used PWA cache `hwf0910at-pwa-v5`.
- The 0.3.16 operational sketch compiled at 1,081,817 bytes (82%) with
  51,684 bytes of global RAM; the public sketch compiled at 1,081,673 bytes.
- Firmware and LittleFS OTA completed with the saved Daily schedule intact.
  After local-time synchronization it resumed `CUSTOM` 100%, schedule driving,
  motor requested, MOC armed, and firing allowed at approximately 120 Hz.
- Periodic status is 4,568 bytes; the 6,578-byte power log and 1,188-byte
  temperature history are fetched separately. Installed source and served PWA
  hashes matched, and the cache is `hwf0910at-pwa-v6`.
- Twelve complete app-entry flows preserved scheduled output. A subsequent
  ten-minute test used three clients polling status every three seconds plus a
  fourth client loading the power log every 30 seconds: 579 status samples, 20
  log requests, zero HTTP errors, zero invalid fan states, zero unstable ZC
  samples, and 71,982 additional TRIAC pulses. Maximum request latency reached
  3.056 seconds without changing AC state; maximum fire gap remained 8,488 us.
- Final installed state was left running from the saved Daily schedule at
  `CUSTOM` 100%, with schedule active/driving, MOC armed, firing allowed,
  120.0 Hz ZC, AC connection count 1, and zero temperature/CRC errors.

## Limitations

No current transformer, tachometer, or TRIAC-output voltage feedback is installed.
Therefore software cannot independently prove blade rotation, delivered RMS
voltage, or TRIAC conduction. Device Health reports prerequisites and command
activity, not a closed-loop motor measurement.

Still pending: extended thermal soak, quantified airflow/noise, calibrated LED
color transfer, electrical waveform captures, and a final operating video.
