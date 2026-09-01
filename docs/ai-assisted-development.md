# AI-Assisted Development

## Disclosure

This project used ChatGPT and Codex in multiple, separate sessions from early
research through implementation and maintenance. AI assistance was material and
is disclosed because it formed part of the engineering workflow.

AI was used to:

- organize reverse-engineering observations and propose testable hypotheses;
- locate relevant questions in original module pinouts and datasheets;
- draft and review ESP32-S3 firmware, HTTP APIs, NVM logic, and safety states;
- develop and debug the LittleFS-hosted HTML/CSS/JavaScript PWA;
- construct build, OTA, browser, and regression-test procedures;
- maintain pin maps, handoff records, diagrams, and portfolio documentation.

## Human Verification And Authority

AI output was never treated as proof that a physical connection or behavior was
correct. Ivves retained authority over the next action and verified proposals
using the original component documentation plus direct evidence from the fan.

Human work included:

- deciding the sequence and acceptable risk of each physical step;
- disassembly, cleaning, component selection, placement, and mechanical layout;
- continuity, resistance, voltage, temperature, and physical-distance checks;
- soldering, insulation, new wiring, replacement of original slider wiring, and
  installation of WAGO distribution and protection components;
- identifying the actual relationship between sliders, CBB61, motors, H11AA1,
  MOC3023, TRIAC, LED chain, sensor, and ESP32-S3 pins;
- energized testing, observing motor/LED behavior, and deciding whether results
  agreed with the physical system;
- questioning AI conclusions and stopping or correcting work when they did not.

AI did not solder, route wires, measure clearances, read a multimeter, inspect a
joint, choose final component placement, operate the fan, or certify mains safety.
It had no independent physical awareness and did not autonomously choose the next
step. Those limitations matter especially in a 120 VAC appliance.

## Corrections As Evidence

The human-in-the-loop process caught meaningful problems, including an early
GPIO21/GPIO47 inversion, incorrect component assumptions, DS18B20 acquisition
issues, a physical LED-wiring short, UI synchronization errors, and unsafe or
incomplete interpretations of device telemetry. The corrections are documented
in [Engineering Decisions](engineering-decisions.md) and the project changelog.

This record does not claim that human review makes every design choice certified
or error-free. It states the provenance honestly: AI accelerated analysis and
software work, while physical evidence and human judgment controlled the build.
