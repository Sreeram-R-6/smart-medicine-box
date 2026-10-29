> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 09 — ESP32 UART and Wi-Fi

Verify actual board labels/flash/native USB and GPIO4/5 availability. C3 GPIO2/8/9 are boot straps; GPIO18/19 are USB. Check flash and onboard LED connections. Regular ESP32 pin assumptions do not apply.

| Source | Destination | Purpose | Voltage |
|---|---|---|---|
| STM32 PA2 TX | ESP GPIO4 RX candidate | STM32 to bridge | 3.3 V |
| ESP GPIO5 TX candidate | STM32 PA3 RX | Bridge to STM32 | 3.3 V |
| STM32 GND | ESP GND | Reference | 0 V |

No power rail connection. Both powered through verified inputs. Bridge UART1 uses 115200 8N1, separate from USB CDC diagnostics. No human text on bridge UART.

Copy secrets.example.h to secrets.h in firmware/esp32 and enter Wi-Fi credentials locally. It is gitignored. Empty SSID skips attempts. No password/token logging.

```powershell
pio run -e esp32 -t upload
pio device list
pio device monitor --port COM8 --baud 115200
```

Replace port. If upload needs bootloader mode: hold BOOT, tap RESET, release BOOT, upload, then reset normally. USB port can change. Firmware never waits for monitor attachment.

Expected: BOOT, backend-not-configured, Wi-Fi connecting, connected IP/RSSI. Fifteen-second attempt deadline, exponential retry delay from 1 to 60 seconds. UART remains serviced. Wi-Fi connected does not mean backend connected. Backend, durable event buffering and sync are pending.
