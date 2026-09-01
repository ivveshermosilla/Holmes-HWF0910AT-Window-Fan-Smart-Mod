# Architecture

The design separates low-voltage control from the fan's mains power path. An
isolated HLK-10M05 supplies the ESP32-S3 domain. H11AA1 optocouplers report the
AC zero crossing and four original slider contacts. A MOC3023 transfers the
controller's firing command back across the isolation boundary to a BTA08 TRIAC.

```mermaid
flowchart LR
  AC[120 VAC input] --> F[Fuse and protection]
  F --> HLK[HLK-10M05 isolated 5 V]
  HLK --> ESP[ESP32-S3]
  F --> ZC[H11AA1 zero-cross]
  ZC --> ESP
  F --> S[Original sliders]
  S --> H[4 x H11AA1 sensing]
  H --> ESP
  ESP --> MOC[MOC3023 optotriac]
  MOC --> TRIAC[BTA08-600C TRIAC]
  TRIAC --> MOTORS[Twin AC motors]
  ESP --> BUF[SN74AHCT125 level shifter]
  BUF --> LEDS[7 x addressable LEDs]
  ESP --> TEMP[DS18B20]
  ESP --> BTN[Original push button]
  ESP <--> PWA[Local PWA / OTA]
```

## Control Layers

1. Interrupt handlers timestamp zero-cross and H11 edges.
2. The main loop derives mains presence, slider position, temperature, and time.
3. A state machine resolves button, web, thermostat, timer, and schedule requests.
4. The phase controller fires a short MOC pulse after a calibrated delay.
5. LED rendering derives its normal state from the resolved fan mode.
6. HTTP APIs expose state and accept explicit commands; page load is read-only.

The MOC pulse counter is command telemetry. It proves that firmware requested a
gate pulse while the safety conditions were satisfied; it cannot prove current
through the TRIAC or motor. That distinction is important in diagnostics.
