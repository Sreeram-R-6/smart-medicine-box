# Validation and troubleshooting

## Reproducible local checks

```sh
python tools/test_host.py
node tools/test_dashboard.mjs
python tools/check_project.py
python tools/render_oled_previews.py
pio run -e stm32
pio run -e esp32
```

The C runner uses GCC by default; pass `--cc clang` or a full executable path if needed. It creates ignored `.pio/` outputs. Node tests use built-in modules and need no npm dependencies.

| Suite | Coverage |
| --- | --- |
| Software I²C | ACK/NACK, repeated-start reads, register writes, stretch/stuck-line bounds and recovery |
| OLED transport | Partial-write readdressing, shadow integrity, retries, reconnect and full redraw |
| Protocol | Shared diagnostic framing/parser and CRC |
| Menu | Navigation, calendar edits, draft cancellation, names/days, save errors, history wrap and 100,000 navigation/render steps |
| Alarm/storage/RTC | Calendar round trips, duplicate suppression, groups, fresh lid cycles, reboot/clock gaps, repeating melody, trust failures, CRC, torn commit/fallback and sequence wrap |
| Control | Every v2 payload length, per-byte corruption, parser timeout, wire packing, configuration revision and boundary validation |
| Screen | Next alarm across midnight/weekdays, grouped names, end-of-calendar bounds, buffer guards, every view and 23 pixel previews |
| Analytics | Persistent response/ringing/interval counters, report conflicts, interruptions, bounded retention, generation matching and schema-1 upgrade |
| Dashboard | Actual shipped script with DOM/network fixtures: safe text rendering, weekday editor, revision/token POST, ACK/readback success and offline controls |

GitHub Actions runs the host checks and compiles both targets. It never flashes connected hardware. A green CI run does not validate electrical wiring, radio range, sound, battery retention or actual pill taking.

## Latest assembly status

The 2026-10-30 rewiring check recovered RTC time and UART communication, but OLED transfers still fail. See [current results and recovery changes](20-rewiring-recovery.md). Earlier successful checks below apply to the previous assembly.

## Bench evidence

The following checks were performed on the attached prototype during development:

- Both firmware images flashed and verified. UART telemetry included all eight saved schedules, lid state, clock health and history.
- A disabled slot was changed through HTTP → ESP32 → UART → STM32 flash → ACK → telemetry readback. An STM32 reset retained it. Original slot contents were restored.
- Invalid time and stale-revision HTTP requests were rejected.
- Brightness settings were updated through the same path and restored.
- Web RTC sync reached three trusted advancing ticks and matched current India time within the measurement interval on main power.
- The ten-second melody remained active through 28 seconds and stopped after a physical lid opening.
- A phone connected to MedBox and opened the dashboard after explicit AP addressing and captive-portal support were added.
- The user confirmed OLED boot animation/menu navigation. A captured display framebuffer showed the next saved reminder's time and name.
- No UART frame, RTC read or OLED write errors were reported during the final integration checks.
- Activity firmware preserved the existing schema-1 schedules/settings on upgrade. A real ACTIVITY TEST reminder measured 13 seconds to opening and 12,295 ms ringing; report changes committed/read back, extra openings counted, and a restart retained measurements and original configuration. See [measurement definitions](19-activity-dashboard.md).

These are development observations, not continuous certification. The RTC backup test **failed**: the battery measured 1.8 V and time reset to year 2000. Its replacement/power-off retention test is still required. Earlier notes include intermediate failures and obsolete firmware behavior; use this guide for current status.

## Physical acceptance checklist

1. Check supply/input levels and common ground before wiring all modules.
2. Exercise LEFT, RIGHT, BACK and OK, including canceling edited values and leaving a schedule unchanged.
3. Compare the FC-51 indicator with OLED OPEN/CLOSED over repeated physical cycles.
4. Close the box; hold BACK+OK for a second. Leave the sensor untouched for at least 25 seconds, then open. Check repeated sound and immediate stop.
5. Change a **disabled spare slot** through the dashboard; wait for confirmed save, reset STM32, check readback, and restore the original slot.
6. Check a scheduled occurrence with a supervised temporary reminder, then remove/restore that test configuration. Do not confuse the standalone test alarm with a real scheduled-occurrence test.
7. Replace a depleted RTC cell, sync, remove main power for at least 30 seconds, restore, wait for clock checks, and confirm correct time without re-syncing.
8. Disconnect Wi-Fi/ESP32 and check local menu/lid operation. Reconnect and verify fresh telemetry; offline web controls must not report saves.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| OLED blank after restart | Power off OLED/RTC/STM32 together briefly, then restore. Check SDA/SCL wiring, 3.3 V pull-ups and addresses 0x3C/0x68. Firmware retries bus recovery. |
| Clock year 2000 / OSF | Verify battery voltage/polarity/contacts and oscillator status, replace a depleted cell, then sync. Do not clear OSF to hide a failing backup supply. |
| IR module LED changes but lid state does not | Check OUT → B12, common GND, actual output voltage and calibration polarity. |
| Buzzer silent or supply dips | Check module supply/driver requirements and ground. This build expects the verified PWM-compatible PB0 arrangement; making a pulse longer does not fix a supply fault. |
| MedBox not visible | Power ESP32 by USB, release BOOT and reset normally. Stay near the board. Firmware advertises on channel 6; `/api/network` exposes AP startup state. |
| Phone says no internet | Stay on MedBox, use its sign-in prompt or explicit `http://192.168.4.1/`; do not use HTTPS. Disable mobile-data auto-switch temporarily if necessary. |
| Phone connected, page unreachable | Reconnect to refresh DHCP. Expected phone address is 192.168.4.x. Check VPN routing or use another browser; AP diagnostics show client IP/RSSI and page hits. |
| Dashboard says STM32 offline | Wire A2 → GPIO4, A3 → GPIO5 and common GND; power both boards. A0/A1/A4/A5 are the button pins. |
| Save reports busy/stale | Finish or cancel the local OLED draft, refresh the web editor, then retry. |
| Save timeout | Outcome is unknown. Read current STM32 state before retrying; do not claim rollback or saved success. |
| Next alarm missing | Check enabled slots/weekdays, clock trust and storage status. Home deliberately shows a warning when time is untrusted. |

For read-only USB diagnostics, install `pyserial` and run:

```sh
python tools/check_device_http.py --port COM20
```

Replace the port. Its `--reset` option briefly disconnects Wi-Fi clients. `Device.change()` makes real hardware changes; restore test values afterward.
