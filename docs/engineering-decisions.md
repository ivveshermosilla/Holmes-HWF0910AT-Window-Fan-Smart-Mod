# Engineering Decisions And Challenges

| Challenge | Evidence | Decision and resolution | Validation |
| --- | --- | --- | --- |
| Original supply was not safely isolated | Board tracing contradicted the early assumption | Replace the control board and power the ESP32 from an HLK-10M05 | Isolated controller domain documented and installed |
| H11 inventory/roles changed during tracing | Five optocoupler functions were found | Assign one ZC plus four explicit slider inputs | Live H11 counters and position labels exposed in System |
| LED wiring produced a short | Physical LED test failed before mains testing | Correct wiring before continuing firmware tests | Seven-pixel color and function tests restored |
| GPIO21 and GPIO47 were reversed in an early build | LED and motor behavior contradicted board wiring | Fix LED DATA=21, MOC=47 and reject stale LED-pin NVS | Correct pin map compiled, flashed, and retained |
| DS18B20 accumulated read errors | Presence/read counters showed repeated failures | Correct one-wire timing and GPIO handling | Stable live readings with zero accumulating errors |
| Low speed needs reliable startup torque | Direct low-power starts were mechanically weak | Apply two seconds at full power only from stopped state | HIGH-to-lower transition observed without app-visible bounce |
| Mobile individual color picker closed early | Re-render replaced the active input | Separate immediate preview from state refresh | Picker remains usable and changes render immediately |
| A second browser briefly altered operation | Client restored stale local controls during startup | Make initial synchronization read-only | New clients observe current state without a motor command |
| Overnight schedules cross weekday boundaries | End time can be numerically earlier than start | Bind weekday to start; carry end into next day | Monday 10 PM to Tuesday 7 AM resolves as one interval |
| 5 GHz network could not be joined | ESP32-S3 scan/join behavior | Provision a 2.4 GHz SSID through AP mode | LAN operation and OTA established |
| OTA must update code and UI independently | App and LittleFS use separate partitions | Provide two upload endpoints and progress/reboot flow | Both image types accepted and served after reboot |

The record deliberately includes failures because the corrective loop is part of
the engineering outcome: observe, isolate the cause, reduce risk, change one
thing, and verify the real device.
