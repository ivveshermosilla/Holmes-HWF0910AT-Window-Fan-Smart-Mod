# Hardware

## Design Boundary

The Holmes HWF0910AT Smart Mod is a selective reuse of a working appliance, not
a drop-in board replacement. The original mains control board was removed. The
two motors, dual CBB61 capacitor, direction sliders, enclosure and mechanical
air path remain part of the fan; the controller, sensing, user button, lighting,
power protection and isolated low-voltage domain were rebuilt.

## Power And Isolation

HOT enters a T1.6A/250 V slow-blow fuse. The protected `L_FUSED` node supplies
the HLK-10M05, MOV, zero-cross detector, slider sensing feeds and TRIAC stage.
The MOV 20D241K is connected between `L_FUSED` and neutral, after the fuse.

The HLK-10M05 creates the isolated 5 V domain. Its output passes through an
external 5 V over-voltage protection module before reaching the motherboard
bus. ESP32, SN74AHCT125, WS2812 LEDs, H11 output transistors and MOC input all
share that isolated domain. A 1000 uF/16 V capacitor is installed across the
protected +5 V and isolated GND bus.

Neutral and isolated GND are never bonded. Signals cross the boundary only
through the five H11AA1 optocouplers and the MOC3023.

## Retained And Replaced Work

The motors, blades, CBB61, both direction-slider mechanisms and enclosure were
retained after inspection and service. The original controller, indicator
electronics, low-voltage supply and physical button were not carried into the
new circuit.

The original yellow, brown, blue and black slider conductors were removed and
replaced with silicone-insulated wire. The replacement improved consistency,
flexibility during routing, and thermal tolerance compared with the original
PVC insulation. Additional slider-sense conductors were added because the
factory appliance did not expose every contact required for full A/OFF/B
reporting.

The new GPIO8 button is mounted on the replacement perfboard. A thin plastic
spacer matching the external plastic actuator diameter, together with Kapton,
sets the operating height. Unneeded original plastic supports were trimmed only
where they obstructed correct closure around the new components.

## TRIAC And Optotriac Network

With the BTA08-600C text facing the viewer and leads pointing down, this project
uses pin 1 as MT1, pin 2 as MT2 and pin 3 as Gate:

```text
GPIO47 -- 220 R -- MOC3023 pin 1
isolated GND ----- MOC3023 pin 2
MOC3023 pin 4 ---- BTA08 pin 3 / Gate
MOC3023 pin 6 -- 200 R / 2 W -- node J
BTA08 pin 2 / MT2 -- 200 R / 2 W -- node J
node J -- 33 nF X2 / 275 VAC -- BTA08 pin 1 / MT1
BTA08 pin 1 / MT1 -- L_FUSED
BTA08 pin 2 / MT2 -- TRIAC_OUT to both slider power commons
BTA08 pin 3 / Gate -- 330 R -- BTA08 pin 1 / MT1
```

The two 200 Ohm resistors form a 400 Ohm path from MT2 to MOC pin 6 through
node J. The capacitor is an X2 mains-rated part. GPIO21 is not part of this
network; it drives the LED buffer.

## Slider Structure

Each original six-pin slider contains two mechanically linked poles. In the
documented top view the terminal layout is:

```text
1  6
2  5
3  4
```

Pins 1-2-3 form the motor-power pole and pins 6-5-4 form the sensing pole. Pin 2
is the power common fed by `TRIAC_OUT`; pins 1 and 3 select the original motor/
CBB61 direction branches. Pin 5 is the sensing common. Position A closes the A
branch, center OFF closes neither branch, and position B closes the B branch.
Power and sensing commons are separate networks.

One 22 kOhm/2 W feed is shared by the two H11AA1 inputs associated with each
slider. Five individual 10 kOhm pull-ups are installed on the isolated H11
collector outputs: one zero-cross channel and four slider channels.

## Physical Condition

The fan was operational when work began, but its internal electrical condition
and undocumented controller behavior were not yet known. Dust and lint had
accumulated during normal extended airflow service, as commonly occurs in a
window fan. The condition was recorded before disassembly, and the enclosure,
grilles and blades were serviced before final integration. The photographic
record documents the improvement without claiming a pristine restoration.

See [BOM](bom.md), [ESP32 Pinout](pinout.md), [Module Connections](module-connections.md)
and [Safety](safety.md).
