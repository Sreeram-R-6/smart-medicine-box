# Build, flash and first run

## Requirements

- STM32F103C8T6 Blue Pill, ST-Link and the components in [hardware](hardware.md).
- ESP32-C3 SuperMini with a working USB-C data cable.
- PlatformIO Core or the PlatformIO extension for VS Code. The project does not require CubeMX or an `.ioc` file.
- Python 3.10+ for project utilities; GCC/Clang and Node.js 18+ only for host tests.

For a Python installation:

```sh
python -m pip install -r requirements-dev.txt
pio run -e stm32
pio run -e esp32
```

Run commands from the repository root. PlatformIO downloads framework/toolchain packages on the first build. Pins and packages are defined in `platformio.ini`:

| Environment | Platform | Framework | Build profile |
| --- | --- | --- | --- |
| `stm32` | `ststm32@19.0.0` | STM32Cube HAL | `bluepill_f103c8` |
| `esp32` | `espressif32@6.10.0` | Arduino ESP32 2.0.17 | `esp32-c3-devkitm-1` |

The ESP32 profile is a build configuration, not a claim that every SuperMini has identical hardware. The tested board had an ESP32-C3 and 4 MB embedded flash. USB CDC is enabled, and bridge UART pins are explicitly GPIO4 RX / GPIO5 TX.

## Upload STM32

With power disconnected, wire ST-Link SWDIO to A13, SWCLK to A14, and GND to GND. Power the Blue Pill through its board input; leave unverified programmer power pins disconnected. Keep BOOT0 low.

```sh
pio run -e stm32 -t upload
```

The custom linker script allocates **56 KiB application flash** and reserves the last **8 KiB of nominal 64 KiB flash** for configuration/history banks. Normal application uploads preserve those banks. A full-chip erase or an incompatible storage format can remove settings. Do not replace this linker script with the framework default without reviewing the storage addresses.

## Upload ESP32

```sh
pio device list
pio run -e esp32 -t upload --upload-port COM20
```

Replace `COM20` with your actual port, such as `/dev/ttyACM0` on Linux. If the port will not open or download mode is not entered, hold BOOT, tap RESET, release BOOT, and retry. If no RESET button exists, hold BOOT while reconnecting USB. Release BOOT and reset normally after uploading. Native USB ports can disappear/re-enumerate during reset.

USB diagnostics use 115200 baud. Do not connect a serial monitor to the same port while uploading.

## First run

1. Power each board through its own USB and connect the [three UART wires](hardware.md#esp32-uart).
2. The OLED plays its boot animation, then shows Home. Press OK for the menu; LEFT/RIGHT navigate and BACK returns.
3. Join **MedBox**, password **medbox-setup**. Select “stay connected” for the no-internet warning.
4. Open **http://192.168.4.1/** or the phone's “Sign in to network” prompt. It is HTTP, not HTTPS.
5. Check the page reports **STM32 connected**. Offline mode disables saves.
6. Use **Sync current India time** after checking the phone/computer clock is correct.
7. Rename and configure the eight reminder slots. A success message requires an STM32 acknowledgment and configuration readback.
8. Close the box, hold BACK+OK for about a second, wait through at least two melody cycles, then open the lid to stop it.
9. Verify clock retention with USB power removed and the RTC battery installed. A running clock while USB is connected is not this test.

Default reminders are daily at 06:00, 10:00, 13:00, 16:00, 20:00 and 23:00 India time, labeled REMINDER 1–6. Slots 7/8 start disabled. These example times are applied only to blank storage; a normal reflash does not overwrite saved schedules.

## Editing the web page

The source is `firmware/esp32/dashboard.html`. Its generated C header is committed so ordinary builds need no generation step. After editing the HTML:

```sh
python tools/embed_dashboard.py
python tools/check_project.py
pio run -e esp32
```

Optional existing-network credentials go in an ignored `firmware/esp32/secrets.h` copied from `secrets.example.h`. Empty credentials keep the board in access-point-only mode. Local AP control does not require home Wi-Fi or internet. Never commit real credentials.
