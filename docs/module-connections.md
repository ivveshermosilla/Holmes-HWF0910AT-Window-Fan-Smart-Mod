# Module Connections

This is the terminal-level connection record for the working prototype. Net
names describe electrical function and intentionally replace historical wire
colors. DIP numbering is top view with the notch or pin-1 mark oriented in the
normal datasheet position. Disconnect mains and verify the actual package
marking before using this record.

## AC Input, Fuse And MOV

| Terminal | Connection |
| --- | --- |
| Plug HOT | Fuse holder input |
| Fuse holder output | T1.6A/250 V slow-blow fuse to `L_FUSED` |
| Plug neutral | `N` distribution node |
| MOV 20D241K lead 1 | `L_FUSED` |
| MOV 20D241K lead 2 | `N` |

The MOV is non-polar and is installed after the fuse. WAGO connectors are used
as separate distribution nodes; different potentials are never combined in one
electrical node.

## HLK-10M05 And 5 V OVP

| Terminal | Connection |
| --- | --- |
| HLK AC/L | `L_FUSED` |
| HLK AC/N | `N` |
| HLK +V | OVP `IN+` |
| HLK -V | OVP `IN-` |
| OVP `OUT+` | Protected isolated +5 V motherboard bus |
| OVP `OUT-` | Isolated GND bus |

The OVP module's exact SKU and set threshold were not preserved in the project
record; `IN+/IN-/OUT+/OUT-` are functional terminal names, not a claim about
its printed silkscreen. A 1000 uF/16 V electrolytic is connected across the
protected +5 V and GND buses with correct polarity.

## ESP32-S3

| ESP32 connection | Destination |
| --- | --- |
| 5 V input | Protected isolated +5 V bus |
| GND | Isolated GND bus |
| GPIO1 | Zero-cross H11AA1 pin 5 collector |
| GPIO2 | Slider H11 channel 1 collector |
| GPIO42 | Slider H11 channel 2 collector |
| GPIO41 | Slider H11 channel 3 collector |
| GPIO40 | Slider H11 channel 4 collector |
| GPIO21 | SN74AHCT125 pin 2 (`1A`) |
| GPIO47 | 220 Ohm resistor to MOC3023 pin 1 |
| GPIO39 | DS18B20 DATA |
| GPIO8 | New push button; other button contact to isolated GND |
| GPIO48 | On-board indicator control, held OFF where supported |

GPIO47 is the MOC command. GPIO21 is the LED data path.

## H11AA1 Zero-Cross Channel

| H11AA1 pin | Connection |
| ---: | --- |
| 1 | `L_FUSED` through two series 22 kOhm/2 W resistors |
| 2 | `N` |
| 3 | NC |
| 4 | Isolated GND |
| 5 | GPIO1 and its own 10 kOhm pull-up to 3.3 V |
| 6 | NC |

## Four H11AA1 Slider Channels

Every slider H11 uses the same isolated output arrangement:

| H11AA1 pin | Connection |
| ---: | --- |
| 1 | Shared 22 kOhm/2 W feed for that physical slider from `L_FUSED` |
| 2 | Its assigned slider A or B sensing contact |
| 3 | NC |
| 4 | Isolated GND |
| 5 | Assigned ESP32 GPIO plus a dedicated 10 kOhm pull-up to 3.3 V |
| 6 | NC |

| Logical channel | Slider contact | GPIO |
| --- | --- | ---: |
| Slider A contact A | SA pin 6 | 2 |
| Slider A contact B | SA pin 4 | 42 |
| Slider B contact A | SB pin 6 | 41 |
| Slider B contact B | SB pin 4 | 40 |

The UI direction labels account for the final software A/B swap. Hardware
diagnostics retain the four explicit contact identities. Both A and B active on
one slider is reported as invalid/transition rather than treated as a valid
direction.

## Original Direction Sliders

Top-view terminal layout:

```text
1  6
2  5
3  4
```

| Pin | Connection/function |
| ---: | --- |
| 1 | Original motor/CBB61 direction branch 1, now on new silicone wire |
| 2 | `TRIAC_OUT` power common, bridged to pin 2 of the other slider |
| 3 | Original motor/CBB61 direction branch 2, now on new silicone wire |
| 4 | B-position sensing contact to its H11AA1 pin 2 |
| 5 | Sensing common `N_SENSE`, bridged to pin 5 of the other slider and `N` |
| 6 | Added A-position sensing contact to its H11AA1 pin 2 |

The original yellow, brown, blue and black conductors associated with these
routes were removed; current wiring uses functional net labels and new
silicone-insulated conductors. Pins 2 and 5 are electrically separate commons.

## MOC3023

| MOC3023 pin | Connection |
| ---: | --- |
| 1 | GPIO47 through 220 Ohm, about 1/4 W |
| 2 | Isolated GND |
| 3 | NC |
| 4 | BTA08 pin 3, Gate |
| 5 | NC |
| 6 | Node J through 200 Ohm/2 W |

## BTA08-600C And Snubber

The installed orientation is text facing the viewer and leads down.

| BTA08 pin | Name | Connection |
| ---: | --- | --- |
| 1 | MT1 / A1 | `L_FUSED`; 330 Ohm to Gate; 33 nF X2 capacitor to node J |
| 2 | MT2 / A2 | `TRIAC_OUT` to both slider pin-2 commons; 200 Ohm/2 W to node J |
| 3 | Gate | MOC3023 pin 4; 330 Ohm to MT1 |

Node J connects to three parts: the 200 Ohm/2 W resistor from MT2, the
200 Ohm/2 W resistor to MOC pin 6, and the 33 nF X2/275 VAC capacitor to MT1.
Do not infer the tab potential or insulation class only from the lead numbering;
verify the exact installed package datasheet and treat the whole area as a
mains-voltage assembly.

## SN74AHCT125

| Pin | Signal | Connection |
| ---: | --- | --- |
| 1 | `/1OE` | Isolated GND; channel enabled |
| 2 | `1A` | ESP32 GPIO21 |
| 3 | `1Y` | 330 Ohm to LED 1 DIN |
| 4, 10, 13 | Unused `/OE` | +5 V; unused channels disabled |
| 5, 9, 12 | Unused inputs | Defined low at isolated GND |
| 6, 8, 11 | Unused outputs | NC |
| 7 | GND | Isolated GND |
| 14 | VCC | Protected +5 V |

A 100 nF X7R capacitor is installed directly between pins 14 and 7.

## WS2812-Compatible LED Chain

Every LED receives protected +5 V and isolated GND, with one 100 nF X7R bypass
capacitor at the device. The data chain is:

```text
SN74 pin 3 -> 330 R -> LED1 DIN
LED1 DOUT -> LED2 DIN -> ... -> LED7 DIN
LED7 DOUT -> NC
```

The physical lead order of the through-hole LEDs depends on the exact part and
was not preserved by SKU, so this document records functional pins rather than
inventing a package lead sequence. LED order is LOW, 60 F, 65 F, 70 F, HIGH,
75 F and 80 F.

## DS18B20

| Sensor lead/function | Connection |
| --- | --- |
| VCC | ESP32 3.3 V |
| DATA | GPIO39 |
| GND | Isolated GND |

DATA is pulled to 3.3 V through the exact installed series chain:

```text
1 k + 1 k + 1 k + 1 k + 680 R + 20 R = 4.700 kOhm
```

## New Physical Button

| Button contact | Connection |
| --- | --- |
| Contact 1 | GPIO8, configured `INPUT_PULLUP` |
| Contact 2 | Isolated GND |

Released reads HIGH and pressed reads LOW. This is a new button on the new
perfboard, mechanically aligned to the original external actuator using a thin
plastic height spacer and Kapton. The original motherboard button remains on
the removed original board.

## Motors And CBB61

The two original motors and dual `2.5 uF + 2.5 uF`, 250 VAC CBB61 remain in the
factory motor-direction network. `TRIAC_OUT` feeds slider pin 2; slider pins 1
and 3 select the corresponding original motor/CBB61 direction branches. Motor
neutral returns remain on `N`. The exact internal motor winding lead identities
were not relabeled as module pins during the project, so no unsupported winding
pinout is claimed here.
