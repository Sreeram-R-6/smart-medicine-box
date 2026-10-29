> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 02 — Identify and measure

No photos are currently available to the author. Module names do not establish electrical specifications.

| Component | Established specification | Supply | Signal/interface | Current/limitations |
|---|---|---|---|---|
| STM32F103C8T6 | Chip: 64 KB flash, 20 KB RAM | Chip 2.0–3.6 V; board input/regulator must be verified | Design uses 3.3 V GPIO | Module load budget pending |
| ESP32-C3 SuperMini-style | C3 family; exact board unknown | Verified board USB input | 3.3 V UART, native USB if wired | RF bursts; never use Blue Pill regulator |
| DS3231 module | RTC chip known; board circuitry unknown | Candidate 3.3 V, verify module | I2C, chip address 0x68 | Battery maintains time only |
| OLED DST-015D-like | GND/VCC/SCL/SDA labels supplied | Unknown | I2C; controller/resolution unknown | SSD1306 128×64 only a candidate |
| FC-51 | One digital reflective sensor | Verify actual module | OUT voltage/polarity unverified | Cannot identify pills or prove ingestion |
| MH-FMD | Active-low marking supplied | Verify actual module | Input/driver unverified | Current, thresholds, reset safety pending |
| Buttons | Four momentary switches | No VCC needed | GPIO-to-GND | Identify permanently connected leg pairs |
| LEDs | Two, optional third | GPIO through resistor | Active-high | Approx. 2 mA target |

Inventory: 2 × 680 Ω resistors (3 with amber), suitable USB data cables for ESP32/ST-Link, Blue Pill power cable, provisional regulated 5 V 2 A total USB supply, breadboard(s), male/male and male/female jumpers, headers if missing, multimeter and a **3.3 V logic USB-to-TTL adapter** for STM32 logs. Battery/driver/protection parts follow identification.

Before module connection record:

- Front/back photographs and chip/regulator markings; actual ST-Link connector labels.
- Power-off I2C pull-up resistances and destination rail. Never measure resistance with power applied.
- Powered module SCL/SDA idle voltage with MCU disconnected: safe for 3.3 V inputs.
- FC-51 supply/current and OUT voltages in both lid states.
- Buzzer supply/current, I/O idle voltage, input thresholds and onboard driver arrangement.
- RTC battery type and charging circuit. Never put CR2032 in an active charging circuit. Rechargeable cells also need a compatible charger, not an assumed generic diode/resistor circuit.

Primary sources: [ST datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf), [Espressif C3 datasheet](https://documentation.espressif.com/esp32-c3_datasheet_en.html), [DS3231 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf). Chip ratings do not certify a third-party board regulator/module. Actual module currents remain unknown until documented or measured.
