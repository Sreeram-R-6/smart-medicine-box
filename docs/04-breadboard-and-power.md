> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 04 — Breadboard and power

Unplug all power before rewiring. Each group of five holes on one side of the centre gap is normally connected; the opposite side is isolated. Long power rails may break halfway. Verify with continuity mode. Bridge a rail break only when both sections should carry the same supply.

Place boards so opposite header rows cannot short through a five-hole strip. Wide boards may need two breadboards or female jumpers. Label GND, 3V3-STM32 and any separate module rail. Keep 5 V distinct.

| Source | Destination | Purpose | Voltage |
|---|---|---|---|
| USB supply | Verified ESP32 USB-C | ESP32 power via regulator | USB 5 V |
| Separate USB cable | Verified Blue Pill power input | STM32 power | Board-specific input |
| STM32 GND | Common GND rail | Reference | 0 V |
| ESP32 GND | Common GND rail | UART reference | 0 V |
| Modules GND | Common GND rail | Module return | 0 V |
| Verified module supply | Module VCC | Conditional after checks | Module-dependent |

Do not power ESP32 from Blue Pill 3.3 V. Do not connect independent USB outputs or separate regulator outputs together. Joining grounds is required for UART. Avoid a powered TX feeding an unpowered MCU: power both before linking signals, or disconnect signals before powering one off.

Budget measured module currents, LED currents and RF bursts against documented regulator ratings. The unknown Blue Pill regulator is not automatically suitable for all modules. Use a separately regulated module rail if required, shared ground, no parallel outputs.
