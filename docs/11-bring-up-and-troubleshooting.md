> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 11 — Staged checks

Use the same diagnostic firmware for current stages. The table describes acceptance checks; observed OLED/RTC results are recorded separately in 07.

| Stage | Instructions/code | Expected | Failure checks |
|---|---|---|---|
| 1 Power/program | 05, STM32 env | BOOT, PB8 toggle | Supply, BOOT0, SWD, log RX/GND |
| 2 Buttons | 06, same program | One pressed/released log | Four-leg switch orientation |
| 3 I2C scan | 07, enable diagnostic flag | RTC 0x68, OLED candidate address | Pull-ups/rail breaks/SCL-SDA |
| 4 RTC read | 07, same program | Read or untrusted warning | Battery/OSF/24-hour mode; setting pending |
| 5 OLED | SSD1306-compatible 128x64 trial | Title, clock, RTC status | Address, bus recovery, controller/resolution |
| 6 FC-51 | 08, same program | Stable raw transition | Surface/ambient light/threshold |
| 7 LED/buzzer | 06/08 | Green toggle; buzzer disconnected/off | Reset-safe driver verification |
| 8 UART | 09, both programs | ACKs on both logs | Crossing, ground, pins, baud |
| 9 Local reminder | Pending application | Unavailable | Raw sensor log is not an outcome |
| 10 Wi-Fi | 09 | IP/RSSI or bounded retries | 2.4 GHz/credentials/power |
| 11 Backend | Pending scope | Explicit unconfigured log | Wi-Fi is separate |
| 12 Recovery | Below | No indefinite waits | Error/drop counters |

Current checks: empty SSID; wrong credentials; UART unplug/reconnect while both boards powered; buttons during UART traffic; sensor held/unstable; missing RTC. Disconnect power before changing module wires. Expect continued button diagnostics and bounded errors. Reset clears diagnostic queues; no persistence claimed.

Future acceptance checks: no ESP/Wi-Fi/UART; invalid RTC; lid open at alarm start; fresh close/open requirement; no duplicate outcome; late opening remains missed; reset during reminder; schedule edits during active occurrence; simultaneous-time group; distinct overlapping groups; midnight/day masks; clock forward/backward; duplicate command/event replay; power-cut journal recovery. These are pending until corresponding code exists.

Common faults: LED reversed/no resistor, board header rows shorted by breadboard strips, broken power rails, charge-only USB, wrong COM port, 5 V OUT/pull-ups, paralleled power outputs, reused USB pins. Wi-Fi brownouts warrant power measurements.
