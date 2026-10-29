> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 03 — Install and build

Install Visual Studio Code and its PlatformIO IDE extension. Open this repository folder. In the PlatformIO terminal:

```powershell
pio run -e stm32
pio run -e esp32
```

Allow initial package downloads. Environments are pinned in platformio.ini:

- STM32: ststm32 19.0.0, bluepill_f103c8, STM32Cube HAL, C.
- ESP32: espressif32 6.10.0, Arduino; esp32-c3-devkitm-1 is a **build profile**, not an assertion that the SuperMini matches that board. Verify actual flash capacity and native USB wiring.

Use `pio pkg list -e stm32` / `pio pkg list -e esp32` to inspect resolved frameworks. No OLED library is selected until identification.

STM32 settings: reset HSI 8 MHz, no assumed external crystal/PLL; SysTick milliseconds; SWD retained/JTAG disabled; USART1/2 115200 8N1; interrupt TX; USART2 interrupt RX into bounded ring; I2C1 100 kHz; buttons input pull-ups; sensor input without pull; buzzer latch HIGH before output mode; LEDs LOW. No DMA/RTOS. I2C diagnostics use short finite blocking HAL calls, not the final nonblocking driver.

No .ioc needed: peripheral init and handlers belong to main.c. Do not paste CubeMX main.c/IRQ files over them. Framework provides startup, HAL and linker. Flash limit is 64 KB; no persistence region is reserved or written in this milestone.

References: [PlatformIO Blue Pill](https://docs.platformio.org/en/latest/boards/ststm32/bluepill_f103c8.html), [STM32Cube](https://docs.platformio.org/en/stable/frameworks/stm32cube.html), [Espressif serial API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html).
