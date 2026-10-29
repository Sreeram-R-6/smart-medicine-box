> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 14 — Easy connect: STM32 to each component

This guide matches the current PlatformIO firmware. Wire by **printed pin name**, not MCU package number or the position of a wire in a picture.

## How to read the pin labels

Use the names printed on your board: **A0, A1, A2, B0, B6**, and so on.

- **A** means the group of pins called Port A. **B** means Port B.
- Technical diagrams and code may add a **P**: **PA1 = A1**, and **PB6 = B6**. They mean the same physical pin.
- **A1 and B1 are different pins.** Match both the letter and the number.
- The numbers do not mean breadboard hole numbers.
- **GND** means ground; **3V3** means 3.3 V power. Neither is an A/B signal pin.

All wiring instructions below use the shorter board labels. The firmware does not need changing.

## Before starting

- Disconnect all power before adding or moving wires.
- Your image is a generic Blue Pill reference, not a verified photograph of your actual board. Check the labels on your board.
- Orient the board as in the image: **USB connector at the top, four-pin SWD header at the bottom, components facing you**.
- With that orientation, B12, A9, B6, B7, B8 and B9 are on the image's left header. B0, A5, A4, A3, A2, A1 and A0 are on the right header. This is a locator for the supplied image only.
- Labels such as 42 beside B6 in the reference are MCU package pin numbers, not breadboard positions. Look for **B6** on the board.
- Breadboard groups of five holes normally connect together on each side of the centre gap. Power rails can have breaks. Verify continuity before use.
- Keep board header rows from shorting through a shared breadboard strip.

**Important:** this code is bring-up firmware. It tests buttons, filtered raw sensor readings, optional I2C/RTC diagnostics, UART and Wi-Fi. It does not yet run dose reminders, render the OLED, save schedules or sound the buzzer.

## Step 1 — Make the common ground

Use one breadboard rail labelled **GND**.

Connect a verified STM32 GND pin to that rail. Other component GND wires will connect there. Check rail breaks; join only sections intended to be the same GND net.

Do not use 3V3, 5V or VBAT as ground.

## Step 2 — Connect ST-Link for programming

Identify the pins on your actual ST-Link first; clone connector layouts vary.

| From actual ST-Link | To STM32 | Notes |
|---|---|---|
| SWDIO | A13 / SWDIO | Bottom debug header in supplied image |
| SWCLK | A14 / SWCLK | Bottom debug header in supplied image |
| GND | GND | Common reference |
| RESET, if exposed and supported | NRST | Optional |
| Power/reference pin | **Do not connect yet** | Identify its actual function first |

Do not assume an ST-Link pin marked 3.3 V is a sense input. It may supply power. Never join an unidentified programmer power output to an already-powered board rail.

Power the Blue Pill through its verified board power input. Keep BOOT0 LOW for normal flash execution. Preserve A13/A14 for programming/debugging.

```powershell
pio run -e stm32 -t upload
```

## Step 3 — Connect optional STM32 serial logging

A separate USB-to-TTL adapter is needed for the implemented STM32 logs. An ST-Link V2 clone is not assumed to provide a serial port.

| From STM32 | To USB-to-TTL adapter |
|---|---|
| A9 — USART1 TX | RX |
| GND | GND |
| A10 | Leave unconnected |

Leave adapter **TX and VCC unconnected**. Verify adapter logic is 3.3 V; a supply jumper does not necessarily select logic voltage.

Open the adapter's serial port at **115200 baud, 8N1**, then press STM32 reset. Expect a BOOT log. Logs do not automatically appear through SWD or the Blue Pill USB connector.

## Step 4 — Connect four buttons

Each button connects one GPIO to GND when pressed. **No VCC wire is used.**

| From STM32 | Through button | Other contact goes to |
|---|---|---|
| A0 | LEFT | GND rail |
| A1 | RIGHT | GND rail |
| A4 | BACK | GND rail |
| A5 | OK | GND rail |

Simple wiring for each button:

```text
STM32 GPIO ---- normally-open button ---- GND
```

For a four-leg button, use continuity mode to identify the two permanently connected pairs. Connect the GPIO to one pair and GND to the other pair. Pressing should join the pairs. Do not choose contacts by appearance alone.

Firmware enables internal pull-ups: released = HIGH, pressed = LOW. No additional external pull-up is required for this initial arrangement. Debounce is 30 ms.

**Check:** serial output should show LEFT/RIGHT/BACK/OK pressed and released once per stable action. If always pressed, check switch orientation and shorts to GND.

## Step 5 — Connect green and red LEDs

Each LED must have its **own** resistor.

| From STM32 | Resistor | LED connection | Return |
|---|---|---|---|
| B8 | 680 Ω | Green LED anode (+) | Green cathode (−) to GND |
| B9 | 680 Ω | Red LED anode (+) | Red cathode (−) to GND |

```text
B8 ---- 680 ohm ---- GREEN anode | cathode ---- GND
B9 ---- 680 ohm ---- RED   anode | cathode ---- GND
```

Long lead usually indicates anode; flat package edge usually indicates cathode. Verify trimmed or unfamiliar LEDs. Resistor can go on either side of its LED, but must be in series.

Example: (3.3 V − 2.0 V LED forward drop) / 680 Ω ≈ 1.9 mA. Actual forward voltage varies.

**Current code:** green toggles every five seconds; red remains off. This is a diagnostic pattern, not a ready/reminder status display.

Optional third LED: B1 is reserved in the plan but is not initialized or driven by current firmware. Leave it disconnected for now.

## Step 6 — Connect DS3231 and OLED only after voltage checks

Both modules share the same I2C wires.

| From STM32 | To DS3231 | To OLED |
|---|---|---|
| B6 — I2C1 SCL | SCL | SCL |
| B7 — I2C1 SDA | SDA | SDA |
| GND rail | GND | GND |
| Verified suitable module supply | VCC | VCC, only if the same supply is suitable; otherwise its own verified supply |

Signal connections:

```text
B6 ----+---- RTC SCL
        +---- OLED SCL

B7 ----+---- RTC SDA
        +---- OLED SDA

GND ----+---- RTC GND
        +---- OLED GND
```

Do not connect VCC until the actual module supply requirements are known. Inspect existing I2C pull-ups and their voltage. SCL/SDA must idle at safe **3.3 V** levels. Do not add duplicate pull-up resistors automatically. Never let a module pull these signals to 5 V.

Leave RTC SQW/32K and unused pins unconnected. The RTC battery belongs only in its intended holder after checking for a charging circuit. Do not charge CR2032. The battery maintains time, not the whole box.

After electrical checks, edit `firmware/stm32/config.h`:

```c
#define ENABLE_I2C_DIAGNOSTICS 1
```

Rebuild/upload STM32. Expect RTC at **7-bit address 0x68**. OLED address is not verified; 0x3C/0x3D are candidates. The code shifts 7-bit addresses left once for HAL calls.

**Current code:** scans once and reads RTC periodically. It does not set RTC time or draw on OLED. Controller/resolution remain unverified; a scan alone cannot identify them. RTC warnings must be resolved before future scheduling is trusted.

## Step 7 — Connect one FC-51 after checking OUT voltage

| From STM32 / supply | To FC-51 |
|---|---|
| B12 | OUT — only after verifying safe output voltage |
| GND rail | GND |
| Verified sensor supply | VCC |

Do not assume a 5 V powered sensor produces 3.3 V OUT. Measure OUT in both optical states with the MCU disconnected. If it can reach 5 V, do not connect it directly. Select protection/translation from the actual output circuit first.

Aim the sensor at a repeatable target on the shared lid. Adjust sensitivity until open and closed give consistently different readings under expected lighting. Test at least twenty cycles. Do not aim at loose pills.

**Current code:** logs stable raw=0 or raw=1 after 100 ms filtering. It does not assume which means open and does not acknowledge a dose. Record the actual polarity. A digital output alone cannot reliably identify every disconnection/stuck fault.

## Step 8 — Leave buzzer disconnected until its interface is verified

The code reserves **B0**. Intended wiring is:

```text
B0 ---- VERIFIED RESET-SAFE INTERFACE ---- MH-FMD I/O
Verified buzzer supply ------------------- MH-FMD VCC
GND ------------------------------------- MH-FMD GND
```

Do not treat B0 → I/O as an approved direct connection yet. Check module supply/current, I/O thresholds, pull-up voltage and existing transistor driver.

The module marking says active-low. Current software sets B0 **HIGH for OFF** before output initialization and never intentionally sounds it. During reset B0 is high impedance, so a verified external off-bias/driver is still needed to ensure silence. Its exact components cannot be chosen without the input-circuit checks.

## Step 9 — Connect ESP32-C3 UART after checking board pins

Verify GPIO4/5 exist and are available on the actual SuperMini-style board.

| From STM32 | To ESP32-C3 | Direction |
|---|---|---|
| A2 — USART2 TX | GPIO4 — UART RX candidate | STM32 → ESP32 |
| A3 — USART2 RX | GPIO5 — UART TX candidate | ESP32 → STM32 |
| GND rail | GND | Common reference |

```text
STM32 A2 TX  ---------------->  ESP32 GPIO4 RX
STM32 A3 RX  <----------------  ESP32 GPIO5 TX
STM32 GND     -----------------  ESP32 GND
```

Both use **115200 baud, 8N1, 3.3 V logic**. TX crosses to RX; do not connect TX to TX. ESP32 USB CDC logs are separate from this UART.

Do not use ordinary ESP32 pinout assumptions. C3 GPIO2/8/9 are boot straps; GPIO18/19 serve USB. Check actual onboard/flash connections before any reassignment.

Power ESP32 through its verified USB-C input, **not Blue Pill 3.3 V**. Do not join board regulator outputs. Power both boards while UART wires are connected; avoid a powered TX feeding an unpowered board.

**Check:** diagnostic frames/ACKs appear on each board's separate log output. Wi-Fi is not required for UART. A diagnostic ACK proves frame receipt, not schedule application.

## Final connection checklist

| STM32 pin | Connect to |
|---|---|
| A0 | LEFT → GND |
| A1 | RIGHT → GND |
| A4 | BACK → GND |
| A5 | OK → GND |
| B8 | 680 Ω → green anode; cathode → GND |
| B9 | 680 Ω → red anode; cathode → GND |
| B6 | RTC SCL + OLED SCL, after I2C voltage checks |
| B7 | RTC SDA + OLED SDA, after I2C voltage checks |
| B12 | FC-51 OUT, after output-voltage check |
| B0 | Verified reset-safe buzzer interface; leave disconnected until verified |
| A2 | ESP GPIO4 RX, after actual board verification |
| A3 | ESP GPIO5 TX, after actual board verification |
| A9 | Optional 3.3 V USB-TTL RX |
| A10 | Unconnected |
| B1 | Reserved optional third LED; unconnected |
| A13 | ST-Link SWDIO |
| A14 | ST-Link SWCLK |
| NRST | Optional supported probe reset |
| GND | Common GND for all connected devices and adapters |

## Final power checks

- Power each MCU board through its verified board input.
- Never power ESP32 from STM32 3.3 V.
- Never join independent USB power outputs or separate regulator outputs.
- Do not assume Blue Pill regulator can supply all modules; verify module currents and regulator capacity first.
- Do not feed 5 V GPIO signals to either MCU.
- Connect modules by printed labels, not by pictured header position.
- Keep unresolved module VCC/OUT/control wires disconnected until verified.

For the full diagram-generation specification, use `13-wiring-diagram-agent-brief.md`. For build/upload details, use documents 03, 05 and 09. No hardware has been tested by the code author.
