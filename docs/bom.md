# Bill Of Materials

This is the documented module-level BOM for the working prototype. It separates
retained appliance hardware from newly installed parts; quantities and markings
must still be checked against the final assembly before reproduction.

## Retained From The Fan

| Qty | Item | Work performed |
| ---: | --- | --- |
| 2 | Original AC motors and blade assemblies | Cleaned, inspected, retained |
| 1 | Dual CBB61, 2.5 uF + 2.5 uF, 250 VAC | Original motor capacitor retained |
| 2 | Six-pin INTAKE/OFF/EXHAUST sliders | Mechanisms retained; several conductors/terminations reworked |
| 1 | Original physical push button | Rewired as an isolated ESP32 logic input |
| 1 set | Enclosure, grilles and serviceable fasteners | Cleaned and reused |
| selected | Brown, yellow and auxiliary-common conductors | Traced and reassigned for slider sensing |

## Added By The Retrofit

| Qty | Item | Purpose |
| ---: | --- | --- |
| 1 | ESP32-S3 development board | Controller and network interface |
| 1 | HLK-10M05 isolated AC/DC module | 5 V power supply |
| 1 | BTA08-600C TRIAC | Motor power control |
| 1 | MOC3023 | Optically isolated TRIAC gate drive |
| 5 | H11AA1 | Zero-cross and four slider inputs |
| 1 | SN74AHCT125 | LED logic-level conversion |
| 7 | WS2812-compatible 5 V LEDs | Original-mode indication and color lighting |
| 1 | DS18B20 waterproof probe | Temperature measurement |
| 1 | MOV 20D241K | Surge suppression |
| 1 | 33 nF X2 275 VAC capacitor | TRIAC snubber |
| 2 | 200 R, 2 W resistors | TRIAC/MOC network |
| 1 each | 220 R and 330 R resistors | MOC input and TRIAC gate network |
| 2 | WAGO 221-415 | Bridged mains distribution blocks |

Also installed were a T1.6A/250 V slow-blow fuse and holder, new insulated
conductors, WAGO distribution jumpers, slider-sense branches that did not
previously exist, terminals, heat-shrink, mounting hardware, and mains-rated
spacing/insulation. The complete original motherboard and its indicator/control
electronics were removed.
