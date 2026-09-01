# Safety

This project contains exposed and switched 120 VAC circuitry. Contact can cause
fatal shock, burns, fire, or damage to connected equipment.

- Disconnect mains and verify de-energization before opening the enclosure.
- Use isolation, current-limited bring-up, appropriate PPE, insulated tools, and
  measurement equipment rated for the circuit category.
- Preserve fusing, strain relief, creepage, clearance, insulation, and mechanical
  retention appropriate to mains voltage and the appliance environment.
- Never connect ESP32/HLK secondary ground to mains neutral.
- Do not rely on Wi-Fi, firmware OFF, or a low MOC output as lockout/tagout.
- Keep the fan OFF after a newly detected VAC connection; require an explicit
  local or web command to start.
- Treat the static-IP and OTA interfaces as trusted-LAN maintenance features,
  not Internet-facing services.
- Change all default credentials before deployment.

This repository documents one prototype and is not a compliance report. No UL,
ETL, CE, or equivalent certification is claimed.
