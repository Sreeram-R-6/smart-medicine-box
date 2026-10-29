> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 06 — Buttons, LEDs and pin allocation

| STM32 pin | Function |
|---|---|
| PA0/PA1/PA4/PA5 | LEFT/RIGHT/BACK/OK |
| PB6/PB7 | I2C1 SCL/SDA |
| PA2/PA3 | USART2 TX/RX |
| PA9/PA10 | Debug TX/reserved RX |
| PB12 | FC-51 OUT |
| PB0 | Conditional buzzer control |
| PB8/PB9 | Green/red LED |
| PB1 | Optional amber reserved, not configured |
| PA13/PA14 | SWDIO/SWCLK |

Baseline uses 13 signal pins plus 2 SWD; optional debug RX and amber total 17. No expander needed. GPIO definitions live in config.h; peripheral pin setup currently lives in main.c.

Identify button permanently connected pairs with continuity mode. Wire GPIO to one pair and GND to the other: PA0 → LEFT → GND, PA1 → RIGHT → GND, PA4 → BACK → GND, PA5 → OK → GND. No button VCC. Internal pull-up means released HIGH/pressed LOW. Debounce is 30 ms. Wrong four-leg orientation can permanently short the input LOW.

PB8 → 680 Ω → green anode; cathode → GND. PB9 → **separate** 680 Ω → red anode; cathode → GND. Resistor may be on either side in series. Long lead usually anode, flat package edge usually cathode; verify trimmed LEDs.

Example red LED: (3.3−2.0)/680 = 1.9 mA. Recalculate for actual forward voltage. Never omit/share resistors.

Current proximity diagnostic: both external LEDs remain off; the OLED gives proximity indication. Planned application: green steady ready, red blink reminder, red steady unreviewed missed event, red double-flash local fault. Priority fault > reminder > missed > ready. Optional amber slow blink network unavailable. Display gives details; no network-loss buzzer.


Firmware 0.7.0 adds OLED button diagnostics: BUTTON: LEFT/RIGHT/BACK/OK while held, BUTTON: MULTIPLE with the held names for simultaneous presses, and BUTTON: NONE plus the last release after letting go. The last event remains visible so quick taps can be checked. Existing 30 ms debounce applies; lid state, RTC and opening tone remain active. button_diag stores the held mask and per-button press/release counts for SWD checks.
