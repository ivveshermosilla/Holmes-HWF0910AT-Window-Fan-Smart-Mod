# ESP32-S3 Pinout

| GPIO | Signal | Connected module | Firmware behavior |
| ---: | --- | --- | --- |
| 1 | `ZERO_CROSS` | H11AA1 ZC collector | Edge interrupt and AC-presence monitor |
| 2 | `H11_1` | Slider A contact A | Position sensing; Exhaust side after final swap |
| 42 | `H11_2` | Slider A contact B | Position sensing; Intake side after final swap |
| 41 | `H11_3` | Slider B contact A | Position sensing; Exhaust side after final swap |
| 40 | `H11_4` | Slider B contact B | Position sensing; Intake side after final swap |
| 21 | `LED_DATA` | SN74AHCT125 input | Seven addressable indicator LEDs |
| 47 | `TRIAC_TRIGGER` | 220 R to MOC3023 input | Timed gate pulse, normally low |
| 39 | `TEMP` | DS18B20 data | One-wire temperature bus with pull-up |
| 8 | `BUTTON` | New perfboard push button | Active-low, debounced, short/long press |
| 48 | `BOARD_LED` | ESP32-S3 onboard RGB/red LED | Best-effort software OFF by default |

## Indicator Order

| LED | Meaning |
| ---: | --- |
| 1 | LOW |
| 2 | 60 F |
| 3 | 65 F |
| 4 | 70 F |
| 5 | HIGH |
| 6 | 75 F |
| 7 | 80 F |

Normal modes light HIGH or LOW plus the selected thermostat LED. Custom speeds
light both LOW and HIGH. OFF lights none. Manual color/enable overrides last only
for the powered session; "Current function" restores mode-driven rendering.

## Isolation Boundary

ESP32 ground belongs only to the isolated 5 V domain. It must never be bonded to
the mains-neutral WAGO connection. Signals crossing the boundary do so through
the H11AA1 or MOC3023 optical interfaces.

For DIP pin numbers, power terminals, slider contacts and passive networks, see
[Module Connections](module-connections.md).
