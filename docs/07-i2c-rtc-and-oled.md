> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 07 — I2C, RTC and OLED

Connect only after supply and pull-up checks in 02.

| Source | Destination | Purpose | Voltage |
|---|---|---|---|
| PB6 | RTC SCL + OLED SCL | Clock | Safe 3.3 V pull-ups |
| PB7 | RTC SDA + OLED SDA | Data | Safe 3.3 V pull-ups |
| Verified supply | Each VCC | Power | Verify each module |
| Common GND | Each GND | Reference | 0 V |

Use printed labels, not picture positions. RTC SQW/32K unconnected. Existing suitable pull-ups are sufficient; parallel pull-ups reduce resistance. A module must not pull the bus to 5 V.

The connected hardware's 3.3 V supply/pull-ups were confirmed by the user, so ENABLE_I2C_DIAGNOSTICS is now 1 in STM32 config.h. Firmware scans 0x08–0x77 once, then reads RTC every second. RTC uses 7-bit address 0x68. The OLED trial probes 0x3C/0x3D; address alone does not identify controller/resolution.

Constants are 7-bit addresses; STM32 HAL receives address shifted left once. RTC I2C errors log, attempt bounded bus recovery, and retry on the next second. Reads include control/status and temperature. Calendar validation includes month length and leap years; 12-hour values are converted to 24-hour display, and the century flag is decoded. An oscillator-stop flag, invalid calendar, or stalled clock makes time untrusted. Normal monitoring does not write RTC time. Firmware 0.5.0 adds an explicit one-shot SWD mailbox: it validates a supplied 2000-2099 calendar, enables the backup oscillator, writes and verifies the seven time registers, then clears oscillator-stop. There is no build-time or boot-time clock overwrite.

The SSD1306-compatible 128×64 trial uses a 1024-byte framebuffer and a small built-in font. It displays a title, RTC date/time, trust status, uptime and read/error counts. Frame data is sent in 16-byte chunks to keep normal loop work bounded. This is a diagnostic screen, not a menu. Visible text and a changing clock were confirmed by the user on 2026-10-29; the precise controller marking remains unidentified.

## Hardware observations, 2026-10-29

- ST-Link programming and flash verification passed.
- Initial I2C transfers timed out after one RTC read. Adding startup bus release (nine clock pulses and STOP) plus an I2C software reset resolved the observed failure. The same bounded recovery runs after transfer errors.
- After the fix: 33 consecutive RTC reads, 33 OLED frame transfers, zero RTC/OLED errors, and advancing seconds. No further recovery beyond startup was needed during that sample.
- RTC reported 2000-01-01, OSF set (status 0x88), and 33.25 C. Communication and ticking work; current date/time is NOT established. Set the clock explicitly before using it for schedules.
- Physical OLED operation confirmed by the user. Battery retention and long-term clock accuracy have not been tested.
- Diagnostic state (`rtc_diag`, `oled_errors`, `oled_frames`, `i2c_recoveries`) and the UART log ring can also be inspected through SWD. They are volatile and reset on reboot.

## Restart recovery, firmware 0.4.1

A single OLED write failure previously set oled_ready=0 permanently, so the screen could stay blank after restart even while IR events and RTC reads continued. The driver now retries address detection and complete initialization every two seconds, clears stale addresses before probing, and redraws the current proximity screen after recovery. A periodic display-on command checks communication when frame transfer is idle. The shared bus now runs at 25 kHz with paced OLED transfers and bounded timeouts.

The physical data line also became stuck LOW during this investigation; a complete power cycle of STM32/OLED/RTC cleared that condition, and the user confirmed visible display operation. Firmware cannot guarantee recovery from every electrically stuck slave; if the bus remains held LOW, power-cycle all connected I2C devices together.

After that power cycle, observed hardware checks passed: three consecutive STM32 resets each initialized and rendered successfully with zero OLED/RTC errors; forcing the disabled state recovered automatically; forcing an OLED address NACK caused one counted error, then automatic reinitialization and resumed frames. CPU fault registers remained zero. IR transitions in both directions were also observed. Test output is saved locally under .pio/oled-recovery-validation.txt.


## Explicit time sync, 2026-10-29

The RTC was set to the host computer's Asia/Kolkata local time (UTC+05:30) through the one-shot rtc_set_request mailbox. Host verification rejected an invalid February 30 request, verified time readback and advancing seconds, and confirmed correct date/time and trusted status after an STM32 reset. The final readback was 2026-10-29 14:09:58, approximately 1.17 seconds behind the host at capture completion. One earlier reset sample reasserted OSF; a subsequent sync with a longer stabilization interval passed. Battery-backed retention through full power removal is still untested.

The local validation/sync script is .pio/sync_rtc.py. It derives symbol addresses from the matching firmware ELF and submits current host time; it does not store a fixed timestamp in firmware. Run only when deliberately resynchronizing the RTC.
