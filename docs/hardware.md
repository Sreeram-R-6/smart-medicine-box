# Hardware and wiring

Wire by printed labels: **A0 = PA0**, **B6 = PB6**. A0 and B0 are different pins. Disconnect power while changing connections.

## Parts

| Part | Tested use |
| --- | --- |
| STM32F103C8T6 Blue Pill | Reminder controller, RTC/OLED bus, buttons, sensor, buzzer |
| ESP32-C3 SuperMini | Local Wi-Fi server and UART bridge |
| ST-Link V2 | STM32 SWD programming |
| 128×64 I²C OLED | SSD1306-compatible, tested at address 0x3C |
| DS3231 RTC | Time/calendar at address 0x68, with backup battery |
| FC-51 IR module | One shared lid's open/closed state |
| Verified three-pin buzzer module | PB0-driven PWM melody; supply/interface verified on the assembly |
| Four tactile buttons | Active-low input with internal pull-ups |
| Optional LEDs / USB-TTL adapter | Bring-up diagnostics, not required for OLED navigation |

An I²C acknowledgment cannot identify an OLED controller. Other controllers or 128×32 modules require driver changes. An FC-51 is a threshold proximity sensor, not a calibrated distance meter.

## Complete pin map

| STM32 printed pin | Signal / connection |
| --- | --- |
| A0 | LEFT button to GND |
| A1 | RIGHT button to GND |
| A4 | BACK button to GND |
| A5 | OK button to GND |
| B6 | RTC SCL + OLED SCL |
| B7 | RTC SDA + OLED SDA |
| B12 | FC-51 OUT, after verifying 3.3 V logic in both states |
| B0 | Verified buzzer I/O |
| A2 TX | ESP32 GPIO4 RX |
| A3 RX | ESP32 GPIO5 TX |
| A13 | ST-Link SWDIO |
| A14 | ST-Link SWCLK |
| A9 TX | Optional 3.3 V USB-TTL RX, 115200 8N1 |
| B8 | Optional 680 Ω resistor to green LED anode; cathode to GND |
| B9 | Optional 680 Ω resistor to red LED anode; cathode to GND |
| GND | Common ground for connected boards/modules |

Leave unused A10 and B1 disconnected. The onboard USB connector does not replace ST-Link in this project.

## Power

Power the STM32 and ESP32 through their own USB inputs. Do not connect the ESP32's 3V3/5V outputs to the STM32 regulator output. The Blue Pill regulator's available current is not verified for Wi-Fi transmit bursts; use a properly sized, independently verified supply if changing this arrangement.

The tested RTC/OLED bus uses 3.3 V-compatible supplies and SDA/SCL pull-ups to **3.3 V**. Verify the voltage range of your modules. A module powered from 5 V can pull I²C or its output up to 5 V; do not connect that directly to an unverified STM32 input. Shared ground is required even with separate USB power.

## Buttons

Buttons connect the signal pin to GND when pressed. They do not connect to VCC. Firmware provides internal pull-ups and 30 ms debounce.

On a four-leg tactile switch, the two legs on each internal contact are already connected. Use a continuity meter to find the two distinct contact pairs, and wire across those pairs so the switch is open when released and closes when pressed. Board orientation alone is not a reliable guide.

## RTC and OLED

| Module pin | Connection |
| --- | --- |
| VCC | Verified compatible supply, 3.3 V on tested assembly |
| GND | Common GND |
| SCL | B6 |
| SDA | B7 |
| RTC SQW/INT, 32K | Disconnected |

RTC and OLED share the same bus. Firmware uses a conservative **25 kHz** I²C clock, bounded transactions, bus recovery and OLED retries. Keep breadboard wiring short and check pull-ups before changing bus speed.

The tested RTC backup cell measured 1.8 V and did not retain time. DS3231 specifies a minimum 2.3 V backup input. Replace a depleted cell and inspect polarity/holder contacts. Use a CR2032 only when the module does not charge it; a charging module needs the appropriate modification or battery type. Re-sync and perform an actual power-off test after replacement.

## Lid sensor

Connect verified OUT to B12 and GND to common ground. Calibrate the FC-51 adjustment screw against the actual lid/target material and distance. Current mapping:

| Stable OUT | Meaning |
| --- | --- |
| 0 / LOW | Lid target detected: closed |
| 1 / HIGH | Target absent: open |

The firmware filters transitions for 100 ms. Close/open the box several times and compare the sensor indicator with OLED lid status. If the module indicator changes but OLED does not, check OUT wiring, common ground and measured logic levels. A reversed physical arrangement requires changing `SENSOR_OPEN_LEVEL` after calibration.

## Buzzer

The tested three-pin module has VCC, GND and I/O. Its verified arrangement is a compatible supply, common GND and I/O on B0. Firmware drives **TIM3 channel 3 PWM**, holds the line HIGH for silence, and plays a sequence of frequencies for the melody. Do not assume every similarly labeled module accepts this circuit or supply. Use an appropriate driver when its I/O/current requirements demand one.

## ESP32 UART

| Source | Destination |
| --- | --- |
| STM32 A2 / PA2 TX | ESP32 pin 4 / GPIO4 RX |
| ESP32 pin 5 / GPIO5 TX | STM32 A3 / PA3 RX |
| STM32 GND | ESP32 GND |

TX crosses to RX. Both use 115200 baud, 8 data bits, no parity, one stop bit, 3.3 V logic. GPIO4/5 are explicitly assigned UART pins; they are not necessarily the pins printed TX/RX on another ESP32 board. **A2/A3 are not button pins.**

## First electrical checks

Verify supplies, common ground, sensor output levels, idle I²C line voltage and the buzzer interface before powering the complete system. Keep BOOT0 low for STM32 normal execution. Leave ST-Link/USB-TTL power pins disconnected unless their function is known. Bench evidence applies to the tested assembly, not every clone module sold under the same name.
