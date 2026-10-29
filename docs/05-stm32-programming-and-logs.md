> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 05 — Program STM32 and view logs

Start with modules and inter-board UART disconnected.

| Source | Destination | Purpose | Voltage |
|---|---|---|---|
| Verified ST-Link SWDIO | PA13 | Programming | 3.3 V logic |
| Verified ST-Link SWCLK | PA14 | Programming clock | 3.3 V logic |
| ST-Link GND | STM32 GND | Reference | 0 V |
| ST-Link reset, if exposed | NRST | Optional reset | Board logic |
| STM32 PA9 | USB-TTL RX | Logs | 3.3 V logic |
| STM32 GND | USB-TTL GND | Log reference | 0 V |

Verify actual probe pinout first. A clone pin marked 3.3 V may be an **output**, not a sense input. Leave power/reference pins disconnected until identified. Never parallel programmer power with a powered regulator. A real reference input, when required, follows that probe's documentation.

USB-TTL must have 3.3 V logic; a supply selector may not change TX levels. For receive-only logging leave adapter VCC and TX disconnected. PA10 is reserved. ST-Link clone VCOM is not assumed; printf does not automatically appear through SWD.

BOOT0 LOW for normal flash boot. Preserve PA13/14 and NRST; PA11/12 USB are unused.

```powershell
pio run -e stm32 -t upload
pio device list
pio device monitor --port COM7 --baud 115200
```

Replace COM7 with adapter port. Open monitor then reset to see BOOT. PB8 external green LED toggles every five seconds when wired. Onboard LED is not used. Normal operation needs board power and BOOT0 LOW; ST-Link can be removed.
