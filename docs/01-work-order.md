> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 01 — Work order

1. Identify modules and record measurements: 02.
2. Install tools and build: 03.
3. Map breadboard and power: 04.
4. Program STM32 and connect log adapter: 05.
5. Add buttons and LEDs: 06.
6. Verify I2C electrical levels, then add RTC/OLED: 07.
7. Calibrate FC-51 and verify buzzer circuit: 08.
8. Verify ESP32 pins, connect UART, configure Wi-Fi: 09.
9. Understand diagnostic protocol: 10.
10. Run staged tests: 11.
11. Review remaining firmware/website work and build validation: 12.

STM32 owns schedules, time, reminders and dose outcomes. ESP32 is a replaceable internet bridge. One FC-51 monitors one shared lid, not individual medicines or swallowing.

Baseline design: eight schedules, weekday masks, 16-character labels, configurable 15-minute acknowledgement window. Snooze/manual acknowledgement/third LED remain off by default. Internet loss must never disable local functions.

Implemented now: programming diagnostics, interrupt-driven UART/log output, button debounce, raw sensor filtering, optional I2C scan/RTC reads, ESP32 Wi-Fi reconnect. Not implemented: OLED rendering, time setting, menus, scheduling, storage, reliable application protocol or backend. Nothing reports fabricated dose outcomes.
