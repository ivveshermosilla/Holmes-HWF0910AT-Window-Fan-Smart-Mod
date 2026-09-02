# Bill Of Materials

This BOM describes the working prototype at module level. The electrical values
are the installed values recorded during assembly; exact package and lead order
must still be checked against each physical part before reproduction.

## Retained Appliance Hardware

| Qty | Item | Work performed |
| ---: | --- | --- |
| 2 | Original AC motors and blade assemblies | Serviced, inspected, retained |
| 1 | Dual CBB61, 2.5 uF + 2.5 uF, 250 VAC | Original motor capacitor retained |
| 2 | Original six-pin Intake/OFF/Exhaust sliders | Mechanisms retained; associated yellow, brown, blue, and black conductors removed and replaced |
| 1 set | Enclosure, grilles, actuator and serviceable fasteners | Serviced and adapted to the new assembly |

The push button on the original motherboard was **not** reused. That board and
button remain together; the Smart Mod uses a newly installed button on the new
perfboard and reproduces the original behavior in firmware.

## Installed Modules

| Qty | Component | Purpose |
| ---: | --- | --- |
| 1 | ESP32-S3 development board | Logic, Wi-Fi, NVM, PWA, scheduling and diagnostics |
| 1 | HLK-10M05, 5 V / 2 A | Isolated AC-to-5 V supply |
| 1 | External 5 V OVP module | Between HLK output and protected 5 V bus; exact SKU/threshold unconfirmed |
| 1 | SN74AHCT125 | 3.3-to-5 V LED data buffer |
| 7 | WS2812-compatible through-hole 5 V LEDs | Function indication and controllable lighting |
| 1 | Waterproof DS18B20 | Temperature sensor on GPIO39 |
| 1 | New momentary push button | Local control on GPIO8 |
| 5 | H11AA1 | One zero-cross channel and four slider-position channels |
| 1 | MOC3023 | Optically isolated TRIAC trigger |
| 1 | BTA08-600C | Shared motor power control |
| 1 | MOV 20D241K | Surge suppression after the fuse |
| 1 | T1.6A, 250 V slow-blow fuse | HOT input protection |
| 1 | 250 V fuse holder | Replaceable input fuse mounting |
| 2 | WAGO 221 connectors | Separate electrical distribution nodes |

## Resistors

| Qty | Value | Rating | Function |
| ---: | ---: | ---: | --- |
| 2 | 22 kOhm | 2 W | Series pair feeding the zero-cross H11AA1 from `L_FUSED` |
| 2 | 22 kOhm | 2 W | One shared feed per slider for its mutually exclusive A/B H11AA1 pair |
| 5 | 10 kOhm | about 1/4 W | One collector pull-up per H11AA1 to 3.3 V |
| 1 | 330 Ohm | about 1/4 W | SN74AHCT125 output to LED 1 DIN |
| 1 | 220 Ohm | about 1/4 W | **GPIO47** to MOC3023 pin 1 |
| 2 | 200 Ohm | 2 W | MOC3023/BTA08 gate and snubber network |
| 1 | 330 Ohm | about 1/4 W | BTA08 Gate to MT1 |
| 6 | 1 kOhm x4, 680 Ohm x1, 20 Ohm x1 | low power | Series DS18B20 DATA pull-up: exactly 4.700 kOhm total |

The DS18B20 pull-up is physically implemented as:

```text
1 k + 1 k + 1 k + 1 k + 680 R + 20 R = 4.700 kOhm
```

The four large slider resistors sometimes implied by a one-resistor-per-H11
diagram are not installed. Each physical slider uses one shared 22 kOhm/2 W
feed because its A and B contacts cannot be closed simultaneously in normal use.

## Capacitors

| Qty | Value | Location |
| ---: | --- | --- |
| 1 | 1000 uF / 16 V electrolytic | Across protected +5 V and isolated GND |
| 7 | 100 nF X7R | One beside each WS2812-compatible LED |
| 1 | 100 nF X7R | SN74AHCT125 pins 14 and 7 |
| 1 | 33 nF / 0.033 uF X2, 275 VAC | MOC/BTA snubber network |

## Wiring And Mechanical Materials

The original yellow, brown, blue, and black slider conductors were removed and
replaced with new silicone-insulated wire. This gave the build consistent wire
quality, greater flexibility during assembly, and better temperature tolerance
than the original PVC-insulated conductors. Additional branches were installed
where the original appliance had no independent position-sense connection.

Other installed materials include perfboard, terminals, heat-shrink, Kapton
tape, a thin plastic button-height spacer, mounting hardware and insulation. A
small number of obsolete plastic supports were cut because they interfered with
correct enclosure closure and were no longer needed by the replacement control
structure.

See [Module Connections](module-connections.md) for terminal-by-terminal wiring.
