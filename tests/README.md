# Host and hardware checks

Requires a host C compiler such as MinGW GCC. From repository root:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Ishared tests/protocol_test.c -o .pio/protocol_test.exe
./.pio/protocol_test.exe
```

These execute the shared diagnostic parser. Current application checks:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Ifirmware/stm32 firmware/stm32/menu.c tests/menu_test.c -o .pio/menu_test.exe
./.pio/menu_test.exe
gcc -std=c11 -Wall -Wextra -Werror -Ishared -Ifirmware/stm32 firmware/stm32/menu.c firmware/stm32/calendar.c firmware/stm32/alarm.c firmware/stm32/rtc_health.c firmware/stm32/persistence.c firmware/stm32/analytics.c tests/alarm_test.c -o .pio/alarm_test.exe
./.pio/alarm_test.exe
gcc -std=c11 -Wall -Wextra -Werror -Ishared -Ifirmware/stm32 firmware/stm32/menu.c firmware/stm32/calendar.c firmware/stm32/remote.c tests/control_test.c -o .pio/control_test.exe
./.pio/control_test.exe
```

Coverage includes menu drafts/navigation, calendar round trips, reminder duplicates/grouping/fresh lid openings, indefinite repeated melody, RTC trust failures, interrupted flash commits, corruption fallback, and protocol v2 length/corruption/validation boundaries. These cannot certify wiring, sound quality, radio range or battery retention.

With the current ESP32 attached, Python plus pyserial can query the real HTTP server and UART telemetry without changing the computer's Wi-Fi:

```powershell
python tools/check_device_http.py --port COM20
```

The optional `--reset` restarts the ESP32, temporarily disconnecting Wi-Fi clients. The default command only reads data. `Device.change()` is available for explicit hardware integration scripts; it makes real changes and waits for acknowledgment/readback. Restore any temporary schedules/settings after testing.

Current hardware evidence and outstanding checks: [local control and validation](../docs/17-local-control-and-validation.md).

`dashboard_test.mjs` exports `checkDashboard(fs, vm)` for Node's filesystem and VM modules. It executes the actual embedded page script with controlled DOM/network fixtures. It covers safe rendering, editor fields, stale-draft revision transmission, confirmed save handling and disabled offline controls. It does not replace a phone browser check.

OLED rendering and next-reminder checks:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Ishared -Ifirmware/stm32 firmware/stm32/menu.c firmware/stm32/calendar.c firmware/stm32/alarm.c firmware/stm32/upcoming.c firmware/stm32/screen.c tests/screen_test.c -o .pio/screen_test.exe
./.pio/screen_test.exe
python tools/render_oled_previews.py
```

This checks upcoming reminders across weekday/midnight boundaries, supported-calendar bounds and grouped alarms, guards framebuffer memory, and produces 23 exact pixel previews for visual review.

The recommended `python tools/test_host.py` entry point runs all eight C suites, including `analytics_test.c`. Activity checks cover real-event timing/count semantics, grouping/deduplication, manual-report conflicts, reboot uncertainty, record rotation, generation matching and schema-1 migration. Dashboard fixtures additionally verify sample-aware averages, lid/report separation, filtering and disabled stale-record controls.

Transport checks exercise ACK/NACK, repeated-start register reads, bounded clock stretching and stuck lines (`soft_i2c_test.c`), plus partially accepted OLED writes, exact-column retries, shadow integrity and reconnect (`oled_driver_test.c`). These use mocked electrical interfaces and do not replace bench acceptance.
