> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# Smart Medicine Box — complete wiring brief for a diagram-generating agent

## 1. Your task

Create clear, editable technical wiring diagrams from this document. This document is self-contained: do not require access to the project source or earlier conversation.

Deliver:

1. An overall system block diagram.
2. A labelled electrical connection diagram showing every connection in the tables below.
3. Separate detail views for power, buttons/LEDs, I2C modules, sensor/buzzer, programming/logging and UART.
4. An SVG suitable for zooming/printing and its editable source. Mermaid is acceptable for the block diagram; use SVG or a schematic tool for individual electrical nets.
5. A verification legend and a short list of unresolved connections.

Use signal-name blocks rather than guessed physical board drawings. Do not generate photorealistic wiring images. Do not invent breadboard coordinates, board header positions, module ratings or driver circuits.

**Important status:** this is a complete connection brief for a provisional design, not electrically final wiring for unidentified modules. Dashed connections below require verification before construction. Do not silently convert them into confirmed connections.

## 2. System architecture

The STM32F103C8T6 Blue Pill is the main controller. It owns RTC time, medication schedules, the OLED interface, four buttons, a single lid sensor, LEDs, buzzer and authoritative dose outcomes.

The ESP32-C3 SuperMini-style development board handles Wi-Fi and future backend communication. It communicates with STM32 over bidirectional 3.3 V UART. Local STM32 reminders must operate without the ESP32 or internet.

Exactly one FC-51 monitors one shared lid. It detects a calibrated optical change, not pill identity, pill count or swallowing. Label it **shared-lid optical sensor**. Records must say “compartment opened” or “dose acknowledged.”

The current project is bring-up firmware. Menus, OLED rendering, scheduling, persistent history and backend are pending. Diagram optional/planned functions distinctly from implemented wiring.

## 3. Components to draw

| Reference | Component | Known labels/details | Unknowns that must remain visible |
|---|---|---|---|
| U1 | STM32 Blue Pill | Stated STM32F103C8T6; 3.3 V GPIO | Board regulator, power connector and exact board construction |
| U2 | ESP32-C3 SuperMini-style | USB-C, BOOT, RESET | Exact variant, header layout, flash size and onboard connections |
| U3 | Four-pin 0.96-inch I2C OLED | GND, VCC, SCL, SDA; marking resembles DST-015D | Controller, resolution, supply, pull-ups; SSD1306 128×64 is not confirmed |
| U4 | DS3231 RTC module | VCC, GND, SCL, SDA; connect by printed labels | Module pull-ups, charging circuit, battery compatibility |
| U5 | One FC-51 reflective IR module | VCC, GND, OUT | Supply, OUT voltage/output circuit, optical polarity |
| U6 | MH-FMD buzzer module | GND, I/O, VCC; active-low marking | Supply/current, input thresholds, onboard driver/reset behaviour |
| SW1–SW4 | Momentary pushbuttons | LEFT, RIGHT, BACK, OK | Four-leg orientation must be continuity-tested |
| D1/D2 | Green/red LEDs | Separate external LEDs | Actual forward voltage |
| R1/R2 | LED resistors | 680 ohm each, provisional | Recalculate if actual LED requires it |
| D3/R3 | Optional amber LED/resistor | 680 ohm provisional | Reserved only; not configured in current firmware |
| J1 | ST-Link V2 | SWD programmer | Actual connector mapping and power/reference-pin function |
| J2 | Optional USB-to-TTL adapter | 3.3 V logic; receive-only logging | Actual adapter logic level must be verified |
| BAT1 | RTC backup battery | Powers RTC backup timekeeping only | Battery type depends on module charging circuit |

No GPIO expander is needed for this allocation.

## 4. Diagram notation and colours

- Solid line: selected signal connection, subject to normal board-label verification.
- Dashed amber line: unresolved module/board condition; do not connect until verified.
- Dotted grey: optional, future or reserved connection.
- Black: common GND.
- Red: 5 V supply only; never use red to imply a 5 V GPIO signal is permitted.
- Orange: verified 3.3 V supply domain.
- Blue: I2C SCL; green: I2C SDA.
- Purple: UART TX/RX, with direction arrows and both endpoint names.
- Label every net; colour alone must not convey meaning.
- Use junction dots only for electrical joins. Crossing lines without dots are not joined.
- Represent unknown module supplies as separate named conditional supply blocks, not as automatically connected rails.

## 5. Authoritative STM32 allocation

Keep these assignments identical across every diagram.

| STM32 pin | Function | Direction/configuration | Status |
|---|---|---|---|
| PB6 | I2C1 SCL | Alternate-function open drain; external verified pull-up | Selected; module connections conditional |
| PB7 | I2C1 SDA | Alternate-function open drain; external verified pull-up | Selected; module connections conditional |
| PA2 | USART2 TX to ESP32 | 3.3 V UART output | Selected |
| PA3 | USART2 RX from ESP32 | 3.3 V UART input | Selected |
| PA9 | USART1 debug TX | 3.3 V UART output | Selected |
| PA10 | USART1 debug RX | Reserved; not connected for receive-only logs | Reserved |
| PA0 | LEFT | Input with internal pull-up | Selected |
| PA1 | RIGHT | Input with internal pull-up | Selected |
| PA4 | BACK | Input with internal pull-up | Selected |
| PA5 | OK | Input with internal pull-up | Selected |
| PB12 | FC-51 OUT | Digital input; no internal pull in current code | Conditional electrical connection |
| PB0 | MH-FMD control | Intended active-low; software initializes HIGH | Driver/off-bias verification required |
| PB8 | Green LED | Active-high output | Selected |
| PB9 | Red LED | Active-high output | Selected |
| PB1 | Optional amber LED | Future active-high output | Reserved; current firmware does not initialize it |
| PA13 | SWDIO | Retain SWD access | Selected |
| PA14 | SWCLK | Retain SWD access | Selected |
| NRST | Reset | Optional probe reset connection | Depends on probe |

Baseline uses 13 application signal pins plus two SWD pins. Optional debug RX and amber LED bring the total to 17 pins including SWD. PA11/PA12 are not allocated; leave board USB circuitry alone. BOOT0 must be LOW for normal flash execution; preserve existing boot circuitry. Do not assign additional functions to these pins.

## 6. ESP32-C3 allocation

| ESP32 signal | Function | Status |
|---|---|---|
| GPIO4 | Hardware UART RX from STM32 PA2 | Candidate; verify exposed and available on actual board |
| GPIO5 | Hardware UART TX to STM32 PA3 | Candidate; verify exposed and available on actual board |
| GND | Common reference | Required |
| USB-C | Board power/programming/native USB CDC if actually wired that way | Verify actual board |

Bridge settings: 115200 baud, 8 data bits, no parity, one stop bit. USB CDC diagnostics are separate from the hardware UART. Do not put debug text on the inter-board binary protocol stream.

C3 restrictions: GPIO2/8/9 are strapping pins; GPIO18/19 are USB Serial/JTAG. Verify flash-connected pins and onboard LED wiring against the actual variant. Do not apply ordinary ESP32 pinouts to C3. The PlatformIO esp32-c3-devkitm-1 build profile does not prove the SuperMini has that board's physical layout.

## 7. Power and ground diagram

Draw the two MCU board power paths separately. These are **not** connections between their regulator outputs.

| ID | Source → destination | Purpose | Voltage/status |
|---|---|---|---|
| P01 | Suitable USB source/port A → ESP32 USB-C | ESP32 board power through onboard regulator | USB 5 V; verify actual board |
| P02 | Suitable USB source/port B → verified Blue Pill board power connector | STM32 board power | Conditional board input identification |
| P03 | STM32 GND → COMMON_GND | Reference | 0 V |
| P04 | ESP32 GND → COMMON_GND | UART reference | 0 V |
| P05 | OLED GND → COMMON_GND | Return | 0 V |
| P06 | RTC GND → COMMON_GND | Return | 0 V |
| P07 | FC-51 GND → COMMON_GND | Return | 0 V |
| P08 | MH-FMD GND → COMMON_GND | Return | 0 V; connect with verified buzzer assembly |
| P09 | Verified OLED supply → OLED VCC | Module power | Dashed; voltage not established |
| P10 | Verified RTC supply → RTC VCC | Module power | Dashed; 3.3 V is a candidate, check module |
| P11 | Verified sensor supply → FC-51 VCC | Module power | Dashed; voltage/current not established |
| P12 | Verified buzzer supply → MH-FMD VCC | Module power | Dashed; voltage/current not established |

Power rules to print alongside the diagram:

1. Never power ESP32 from Blue Pill 3.3 V regulator.
2. Never parallel independent USB 5 V outputs or separate 3.3 V regulator outputs.
3. Share grounds for UART and all connected signals.
4. Module power may use a verified suitable rail only after summing load currents and checking regulator capacity. Do not silently assign all modules to Blue Pill 3.3 V.
5. A provisional 5 V 2 A total USB supply is an inventory suggestion, not a measured load calculation.
6. Do not feed 5 V signals into either MCU's GPIO in this design.
7. Avoid powered UART/module outputs driving an unpowered MCU. Disconnect signals or design verified isolation/protection for independent power operation. Shared ground does not prevent back-powering.
8. Draw no power connection from the log adapter or unidentified ST-Link power pin.

## 8. RTC and OLED wiring

| ID | Source → destination | Purpose | Voltage/status |
|---|---|---|---|
| I01 | STM32 PB6 → RTC SCL | I2C clock | Dashed until pull-ups verified |
| I02 | STM32 PB7 → RTC SDA | I2C data | Dashed until pull-ups verified |
| I03 | STM32 PB6 → OLED SCL | Same shared I2C clock net | Dashed until module verified |
| I04 | STM32 PB7 → OLED SDA | Same shared I2C data net | Dashed until module verified |
| I05 | Verified safe 3.3 V pull-up arrangement → SCL | Open-drain idle HIGH | Inspect existing resistors first |
| I06 | Verified safe 3.3 V pull-up arrangement → SDA | Open-drain idle HIGH | Inspect existing resistors first |

Power/ground are P05/P06/P09/P10. Connect using module labels, never assumed connector order. Leave RTC SQW/32K and other unallocated module pins unconnected.

Draw existing module pull-ups as **unknown existing pull-ups — inspect**, not as extra required resistors. Do not add duplicate pull-ups automatically. Both signal nets must idle at safe 3.3 V levels. If a module needs a different supply, verify its pull-up rail and whether translation is necessary; do not invent a level-shifter circuit without those measurements.

DS3231 address: 7-bit 0x68. OLED address: unknown; 0x3C/0x3D are candidates only. An I2C scan cannot establish controller/resolution. Firmware constants are 7-bit; STM32 HAL calls shift left once. Do not put shifted HAL addresses on the physical diagram.

BAT1 connects only through the module's intended battery holder, subject to charging-circuit identification. CR2032 must not be charged. Rechargeable batteries require a compatible charge circuit. RTC battery does not power OLED, MCU, sensor or whole medicine box.

## 9. Four-button detail

| ID | Connection | Purpose | Voltage/logic |
|---|---|---|---|
| B01 | PA0 → SW1 LEFT contact A | Input | 3.3 V internal pull-up |
| B02 | SW1 contact B → COMMON_GND | Press pulls LOW | 0 V |
| B03 | PA1 → SW2 RIGHT contact A | Input | 3.3 V internal pull-up |
| B04 | SW2 contact B → COMMON_GND | Press pulls LOW | 0 V |
| B05 | PA4 → SW3 BACK contact A | Input | 3.3 V internal pull-up |
| B06 | SW3 contact B → COMMON_GND | Press pulls LOW | 0 V |
| B07 | PA5 → SW4 OK contact A | Input | 3.3 V internal pull-up |
| B08 | SW4 contact B → COMMON_GND | Press pulls LOW | 0 V |

No button VCC wire. No external pull-up required for this initial arrangement. Show the MCU internal pull-up symbol or label, not a falsely required external component.

For four-leg buttons, two legs are permanently connected to each contact. Identify pairs using continuity testing; switching connects the two contacts. Do not choose pairs by guessed physical orientation. Debounce: 30 ms. Released HIGH, pressed LOW.

## 10. LED detail

| ID | Connection | Purpose | Voltage/current |
|---|---|---|---|
| L01 | PB8 → R1 680 Ω → D1 green anode | Green drive | 3.3 V GPIO, active HIGH |
| L02 | D1 green cathode → COMMON_GND | Return | 0 V |
| L03 | PB9 → R2 680 Ω → D2 red anode | Red drive | 3.3 V GPIO, active HIGH |
| L04 | D2 red cathode → COMMON_GND | Return | 0 V |
| L05 | PB1 → R3 680 Ω → D3 amber anode | Optional future indicator | Dotted grey; not active in current firmware |
| L06 | D3 amber cathode → COMMON_GND | Optional return | Dotted grey |

One resistor per LED; never share one between LEDs. Resistor can be on either side in series. Mark A/K explicitly. Long lead usually anode, flat edge usually cathode; verify actual parts.

Calculation example: red LED Vf ≈ 2.0 V gives I = (3.3−2.0)/680 ≈ 1.9 mA. Vf is an example, not an identified part specification.

Current firmware green toggles every five seconds; red off. Future meanings: green ready; red blink reminder; red steady missed; red double-flash local fault. Optional amber indicates network unavailable. No network-loss buzzer.

## 11. FC-51 sensor detail

| ID | Connection | Purpose | Voltage/status |
|---|---|---|---|
| S01 | FC-51 OUT → PB12 | Lid optical state | Dashed: verify safe output voltage/circuit |
| S02 | Verified sensor supply → FC-51 VCC | Power | P11; dashed |
| S03 | FC-51 GND → COMMON_GND | Return | P07 |

Draw one sensor only. Do not label OUT as guaranteed active-low or active-high. Annotate **open/closed polarity determined by calibration**. Current firmware input has no internal pull and filters changes for 100 ms.

If module requires 5 V and OUT reaches 5 V, do not show direct connection as approved. Add a dashed placeholder **output protection/translation TBD after output-circuit identification**, not guessed component values. A disconnected digital line can look like a valid state; do not claim reliable disconnect detection.

Show an optional small mechanical illustration with the sensor looking at a fixed lid target. No pill-counting beam or multi-compartment identification. Calibrate under varied ambient light with repeatable target surface/distance.

## 12. MH-FMD buzzer detail

| ID | Logical connection | Purpose | Status |
|---|---|---|---|
| Z01 | PB0 → verified control interface → MH-FMD I/O | Active-low control | Dashed; direct GPIO compatibility not established |
| Z02 | Verified buzzer supply → MH-FMD VCC | Power | P12; supply/current unknown |
| Z03 | MH-FMD GND → COMMON_GND | Return | P08 |

**Do not draw PB0 directly wired to I/O as a confirmed connection.** The control interface placeholder may become a direct connection with verified off bias or a driver after measurement. Do not invent transistor type, resistor values, flyback diode or supply.

Required behaviour: LOW triggers according to module marking; intended software OFF is HIGH. PB0 is high impedance during reset. Software writing HIGH after startup alone does not guarantee reset silence. The final verified bias/interface must keep buzzer OFF when STM32 is reset, absent or starting. If the module input is pulled to 5 V, protect MCU accordingly. Verify driver already fitted before adding another.

Current firmware never intentionally sounds buzzer. Leave module disconnected until checked. Planned audible pattern is intermittent, nonblocking, only during reminders.

## 13. UART detail

| ID | Source → destination | Purpose | Electrical/settings |
|---|---|---|---|
| U01 | STM32 PA2 / USART2 TX → ESP GPIO4 / UART RX | Controller to bridge | Dashed until ESP board verified; 3.3 V |
| U02 | ESP GPIO5 / UART TX → STM32 PA3 / USART2 RX | Bridge to controller | Dashed until ESP board verified; 3.3 V |
| U03 | STM32 GND ↔ ESP GND | Signal reference | Required; COMMON_GND |

Show explicit arrows: TX to RX in both directions. Do not join TX-to-TX. 115200 8N1. These are logic UART wires, not RS-232 voltage levels. No voltage-divider/level-shifter is required for verified 3.3 V UART signals on powered boards; independent power-state protection is a separate unresolved concern.

Label binary protocol UART separately from human-readable logs. ESP USB CDC does not share these GPIO4/5 wires.

## 14. Programming and log detail

| ID | Source → destination | Purpose | Status |
|---|---|---|---|
| D01 | Actual probe SWDIO → PA13 | Debug data | Verify probe pin label |
| D02 | Actual probe SWCLK → PA14 | Debug clock | Verify probe pin label |
| D03 | Probe GND → COMMON_GND | Reference | Required |
| D04 | Probe reset → NRST | Optional connect-under-reset | Only if actually exposed/supported |
| D05 | Probe power/reference → board | Not specified | **Do not connect/draw a wire until identified** |
| D06 | STM32 PA9 / USART1 TX → USB-TTL RX | Human-readable logs | 3.3 V logic, 115200 8N1 |
| D07 | USB-TTL GND → COMMON_GND | Log reference | Required for adapter |
| D08 | USB-TTL VCC | Unconnected | Mark NC |
| D09 | USB-TTL TX | Unconnected | Mark NC; PA10 reserved |

Do not assume a clone ST-Link pin labelled 3.3 V is a sense input; it may supply power. Do not parallel probe power with board power. An actual required voltage-reference input needs that probe's documented connection. ST-Link clone virtual COM and SWO support are not assumed. No logs automatically appear through SWD.

Adapter must use 3.3 V logic; an adapter supply jumper does not necessarily change TX levels. ESP logs use its USB-C/native CDC if the board exposes it, not this adapter.

## 15. Breadboard construction notes for diagram captions

- Groups of five holes on each side of centre gap are usually connected; opposite sides are separate.
- Long power rails can break halfway; verify continuity. Bridge only sections belonging to the same named supply.
- Do not short opposite board header rows by placing them on a shared five-hole group.
- Wide boards may require separate breadboards or female jumper leads.
- Use printed module labels. No physical pin order or breadboard coordinate is supplied by this document.
- Disconnect power before rewiring. Check shorts/polarity before powering.
- Keep a shared GND plan and label every supply rail by voltage/domain.

## 16. Assembly order to show as numbered callouts

1. Verify board/probe labels and power paths; leave all modules disconnected.
2. Connect SWD and receive-only STM32 log adapter; program/reset STM32.
3. Add separate-resistor LEDs and four buttons; verify logs.
4. Measure module supply/pull-ups; then connect I2C modules and scan.
5. Verify FC-51 supply/OUT, connect and calibrate raw states.
6. Identify MH-FMD interface and implement reset-safe OFF before connecting.
7. Verify ESP32 GPIO4/5, power both boards and connect crossed UART/common GND.
8. Verify bidirectional UART independently of Wi-Fi.
9. Configure ESP32 Wi-Fi; never treat Wi-Fi as proof of backend connectivity.

## 17. Mandatory diagram review

- All pin names match the authoritative table.
- Both UART arrows cross TX to RX correctly.
- SWD pins preserved; no conflicting allocation.
- Exactly one sensor, four buttons and two required LEDs.
- Optional amber is visibly optional.
- Every LED has its own resistor.
- All buttons switch to GND, with internal pull-ups labelled.
- I2C SCL/SDA share nets with junction dots, safe pull-up voltage, no invented pull-up resistors.
- Unknown OLED/controller/supply remains unknown.
- Buzzer driver/off-bias remains conditional, including reset behaviour.
- RTC battery does not connect to general power rail.
- No ESP32 power from Blue Pill 3.3 V; no joined regulator/USB outputs.
- No invented ST-Link power/reference connection.
- Conditional/unverified connections are unmistakable.

## 18. Primary references and specification boundaries

- STM32 chip electrical limits, memory and pin functions: https://www.st.com/resource/en/datasheet/stm32f103c8.pdf
- ESP32-C3 chip pin restrictions and electrical limits: https://documentation.espressif.com/esp32-c3_datasheet_en.html
- DS3231 chip supply/backup/I2C behaviour: https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf

These describe chips, not unidentified third-party module regulator, charger, pull-up or connector circuits. If obtaining board documentation, match the exact variant. Do not use a similar-looking board to certify this hardware.

**Final output instruction:** render the supplied selected connections faithfully and place unresolved interfaces in clearly labelled dashed boxes. Finish with a short “verify before building” legend. Do not resolve missing electrical information by guessing.
