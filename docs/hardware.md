# Hardware

## Main Modules

| Module | Role | Notes |
| --- | --- | --- |
| ESP32-S3 controller | Logic, Wi-Fi, PWA, NVM, scheduling | Exact dev-board SKU was not confirmed |
| HLK-10M05 | Isolated 120 VAC to 5 V supply | Keeps controller ground isolated from neutral |
| BTA08-600C | Main AC switching TRIAC | Drives the shared motor power path |
| MOC3023 | Random-phase optotriac | Transfers gate commands across isolation |
| H11AA1 x5 | AC optocoupler sensing | One zero-cross plus four slider contacts |
| SN74AHCT125 | 3.3-to-5 V LED data buffer | Drives the addressable indicator chain |
| WS2812-compatible LEDs x7 | Function indicators | Through-hole parts; exact SKU unconfirmed |
| DS18B20 | Digital temperature sensor | One-wire acquisition on GPIO39 |
| MOV 20D241K | Surge suppression | Mains protection network |
| 33 nF X2, 275 VAC | Snubber capacitor | Used with the TRIAC resistor network |
| WAGO 221-415 | Protected mains distribution | Mechanical connection in the enclosure |
| Original push button | Local control | Reused only as an isolated ESP32 logic input |
| Original sliders | Direction and position | Six-pin INTAKE/OFF/EXHAUST mechanisms retained |
| Original motors/CBB61 | Airflow power stage | Existing motors and dual capacitor retained |

## Retained, Reworked, And Added Hardware

The retrofit was selective reuse, not a drop-in controller swap and not a
complete rebuild of every mechanical part.

**Original parts retained:**

- Both AC motors, blades, grilles, enclosure, and serviceable fasteners.
- The original dual-section CBB61 motor capacitor, marked `2.5 uF + 2.5 uF`,
  `250 VAC`, `50/60 Hz`, `MAX TEMP 70 C`.
- Both six-pin mechanical direction sliders and the original push button.
- Selected original conductors whose route and condition were physically
  confirmed, including brown/yellow auxiliary slider conductors repurposed for
  position sensing and the former auxiliary common repurposed as `N_SENSE`.

**Original parts removed or replaced:**

- The complete original motherboard, undocumented controller, non-isolated
  low-voltage supply, TRIAC-control logic, and original indicator electronics.
- Original wiring and terminations at multiple slider/controller points where
  the new power distribution, isolation, or sensing topology required it.
- The original indicator lamps were replaced by seven addressable 5 V LEDs.

**New work and components:**

- New conductors were added where the original fan had no independent slider
  sensing route, including the additional pin-6 branches for the H11AA1 inputs.
- New fused line distribution, MOV protection, WAGO 221 connectors, isolated
  HLK supply, ESP32-S3 controller, optocouplers, MOC/TRIAC stage, LED level
  shifter, addressable LEDs, DS18B20, resistors, capacitors, insulation, and
  mechanical mounting were installed.

No statement that a part was "retained" should be read as meaning its original
wiring remained untouched. Several slider conductors and terminations were
deliberately replaced or reassigned after continuity tracing.

## TRIAC And Optotriac Network

```text
ESP GPIO47 -- 220 R -- MOC3023 pin 1
isolated GND --------- MOC3023 pin 2
MOC3023 pin 4 -------- BTA08 gate
MOC3023 pin 6 -- 200 R / 2 W -- node J
BTA08 MT2 ------ 200 R / 2 W -- node J
node J --------- 33 nF X2 ------- BTA08 MT1
BTA08 MT1 ------ fused line
BTA08 MT2 ------ switched output to sliders/motors
BTA08 gate ----- 330 R ----------- MT1
```

The exact installed resistor, fuse, and spacing values must be verified against
the physical assembly before any reproduction. Photographs are evidence of this
specific prototype, not a manufacturing drawing.

## Physical Condition

The fan arrived operational but with the dust and lint expected after extended
airflow duty. Its condition was recorded before disassembly. Housing, grilles,
and blades were then cleaned before final integration; the before/after record
is preserved in [Gallery](gallery.md).
