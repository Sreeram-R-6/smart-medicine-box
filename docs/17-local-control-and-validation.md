# Local control and validation

Current firmware: 1.0.0-local-control. This guide supersedes historical RAM-only and diagnostic-only limitations in earlier documents.

## Operation

Six daily generic reminders: 06:00, 10:00, 13:00, 16:00, 20:00 and 23:00 India time. Slots 7/8 are disabled. Edit names, times, weekdays and enabled states through the OLED or web page.

LEFT/RIGHT navigate, OK selects/edits, BACK returns. Hold BACK+OK about one second for a test alarm. Its twenty-note melody lasts ten seconds and repeats until a fresh closed-to-open lid transition. An already-open box must be closed and opened. BACK/OK no longer silence the alarm. Active medicine alarms no longer expire automatically; a lid opening acknowledges all armed active groups. This records lid acknowledgment, not medicine ingestion. Pending medicine alarms survive reboot; test alarms are temporary.

New reminder occurrences require trusted time. After a gap, the most recent occurrence per slot within fifteen minutes can become active; older occurrences are logged as downtime uncertainty. Duplicate occurrence timestamps are suppressed. Opening-tone mute does not mute medicine alarms.

Eight schedules, sound/display settings, 32 history entries and alarm checkpoints use alternating CRC-protected 4 KiB flash banks at 0x0800E000/0x0800F000. The linker reserves these 8 KiB within nominal 64 KiB flash. The old committed bank remains intact during an update. Ordinary history auto-saves are coalesced; an immediate power cut can lose a newly added lid event. Schedule success requires a verified flash commit.

## Confirmed RTC hardware fault

On 2026-10-29 SWD showed date 2000-01-01 and OSF=1. The user measured the battery at 1.8 V with main power off; DS3231 specifies VBAT >= 2.3 V. Replace the depleted cell with the correct type for the module and inspect holder polarity/contacts. A CR2032 requires a disabled charging circuit.

Sync validates the calendar, enables battery-backed oscillation, writes and verifies time, waits for ticking, then clears OSF. Trust requires three subsequent advancing ticks. Read errors, invalid dates, unexpected jumps, stalls and OSF prevent new scheduled alarms. A running clock on USB does not certify its backup battery. After battery replacement, synchronize, remove main power for at least 30 seconds, restore it, and confirm correct time/CLOCK OK without another sync.

The web Sync button uses the phone/computer clock converted to India civil time. Check that device's own time first. No cloud or internet clock source is required.

## Wiring and web

STM32 PA2 TX → ESP32 GPIO4 RX; PA3 RX → GPIO5 TX; GND → GND. UART is 115200 8N1 with 3.3 V logic. Power each board by its own USB, leaving regulator outputs separate. A0/A1/A4/A5 remain LEFT/RIGHT/BACK/OK to GND.

Wi-Fi: **MedBox**, password **medbox-setup**, visible 2.4 GHz channel 6. IP/gateway: **192.168.4.1**, subnet 255.255.255.0. Power saving is off; bench transmit power is 11 dBm. DHCP, local DNS and captive-portal routes help phones open **http://192.168.4.1/**. Stay connected despite the no-internet warning. There is no HTTPS endpoint.

The STM32 owns configuration. Protocol v2 has bounded payloads, CRC16 and 32-bit transaction IDs. The ESP32 retries one outstanding operation; the STM32 caches its last mutation result while running. A configuration checksum rejects stale edits, and active OLED drafts block conflicting remote writes. A reset can lose the response cache, so uncertain operations require readback. The web page reports success after ACK and configuration readback. Timeouts are explicitly unknown outcomes.

GET /api/state and /api/network provide telemetry. POST /api/slot, /api/settings, /api/clock and /api/test require the current revision and per-boot X-Medbox-Token. No external assets or cloud calls are needed. Wi-Fi access controls local access; no internet exposure is configured.

The USB maintenance console accepts GET /api/... or POST /api/... followed by URL-encoded fields. It forwards through loopback TCP to the same HTTP server with the token. It does not bypass validation, UART or storage. `tools/check_device_http.py` uses it without switching the computer's Wi-Fi. This verifies actual HTTP/UART hardware paths, not phone rendering.

## Hardware checks completed

- Both builds and firmware flash verification passed.
- Bidirectional UART telemetry includes all eight real schedules, lid state, history and diagnostics. No malformed frames or I2C errors reported during integration tests.
- Invalid time and stale configuration requests were rejected.
- A disabled slot changed through HTTP → ESP32 → UART → STM32 flash → ACK → readback; an STM32 reset retained it. The original disabled slot was restored.
- Brightness changed through the same path and original settings were restored.
- Web clock sync verified success, reached three trusted ticks, and matched current India time within the sampling interval on main power.
- The repeating test remained active through 28 seconds and stopped after an actual sensor opening.
- After explicit AP IP settings, local DNS and captive-portal support, the user confirmed the dashboard opens. ESP32 diagnostics independently showed one phone at 192.168.4.2, RSSI -44 dBm and successful page requests.

The shipped dashboard script also passed a DOM-fixture check of eight schedule rows, safe text rendering, weekday editing, POST token/revision handling, acknowledgment/readback success, and disabled offline controls. This is a functional script test, not a visual layout audit.

Battery-backed time retention failed. Replace the battery and repeat the physical power-off test. Human checks of sound quality and phone usability complement firmware counters.

Sources: [DS3231 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf), [Espressif Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html), [official captive portal example](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/DNSServer/examples/CaptivePortal/CaptivePortal.ino).
