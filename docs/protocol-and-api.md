# UART protocol and HTTP API

The current application uses **protocol v2**, implemented in `shared/control.h`. The original fixed-length diagnostic v1 remains in `shared/protocol.h` for its CRC/parser tests and historical notes. Both production firmwares speak v2.

## UART framing

115200 8N1, 3.3 V logic. All multibyte integers are unsigned little-endian unless stated otherwise.

| Byte offset | Size | Meaning |
| --- | --- | --- |
| 0 | 2 | Sync A5 5A |
| 2 | 1 | Version 02 |
| 3 | 1 | Message type |
| 4 | 1 | Payload length N, 0–208 |
| 5 | 4 | Transaction ID; telemetry/hello can use zero |
| 9 | N | Payload |
| 9 + N | 2 | CRC16-CCITT-FALSE, polynomial 0x1021, initial 0xFFFF |

CRC covers bytes 2 through the final payload byte, **N + 7 bytes**. Total frame size is N + 11, at most 219 bytes. The parser rejects bad versions, overlength payloads and CRC failures; partial frames expire after 100 ms when the next byte arrives.

| Type | Name | Payload |
| --- | --- | --- |
| 16 | HELLO | Empty; sent by ESP32 about once per second |
| 17 | STATE | STM32 telemetry, 194 bytes |
| 18 | SLOT | Expected config CRC16, slot index, 21-byte schedule |
| 19 | SETTINGS | Expected config CRC16, 4-byte settings |
| 20 | CLOCK | Expected config CRC16, uint32 civil epoch |
| 21 | TEST | Expected config CRC16 |
| 22 | RESULT | Result byte, current config CRC16 |
| 23 | HISTORY | One-byte newest-first history index |
| 24 | EVENT | Index, event type, slot mask, dated flag, epoch, due epoch, uptime |
| 25 | ANALYTICS | Empty request for metadata, or mode/index/generation; tagged metadata, slot totals, dose records or intervals |
| 26 | REPORT | Expected config CRC16, uint32 record ID, new report byte, previous report byte |

Schedule wire format is **17 name bytes** (NUL-terminated, at most 16 visible uppercase letters/numbers/spaces), then one byte each for hour, minute, weekday mask and enabled. Monday is bit 0, Sunday bit 6. Settings are opening-tone enabled, brightness 1–10, and uint16 opening-tone duration 100–2000 ms in 100 ms steps.

Civil epochs are seconds since **2000-01-01 00:00:00 in the configured India civil clock**, not Unix UTC timestamps. Maximum supported value is 3155759999 (2099-12-31 23:59:59). To sync from Unix seconds: `unix + 19800 - 946684800`.

## State payload

| Offset | Meaning |
| --- | --- |
| 0–3 | Civil epoch, or zero when invalid |
| 4–5 | Configuration CRC16 |
| 6 | Flags: bit 0 calendar valid, bit 1 trusted, bit 2 lid open, bit 3 storage OK, bit 4 OSF, bit 5 test alarm active |
| 7 | Active reminder group count |
| 8–11 | Settings, as above |
| 12–179 | Eight 21-byte schedules |
| 180–183 | Monotonic uptime in ms |
| 184–187 | RTC read-error count |
| 188–191 | OLED write-error count |
| 192 | History count |
| 193 | Advancing-clock progress, up to 3 |

STM32 sends state about once a second while linked. ESP32 considers telemetry fresh for four seconds; STM32 times out the peer after fifteen seconds. Configuration revision is a checksum of all schedules/settings, not a monotonically increasing generation or a cryptographic signature.

## Result codes and save semantics

| Code | Meaning |
| --- | --- |
| 0 | Confirmed success |
| 1 | Invalid command/value |
| 2 | Busy, including a conflicting local edit |
| 3 | Stale configuration or transaction conflict |
| 4 | Flash save failed |
| 5 | RTC write/tick verification failed |

The ESP32 also reports local result **6** when retries end without an acknowledgment; that is an unknown outcome, not proof the STM32 rejected the change. It attempts up to five sends at roughly two-second intervals. One mutation is in flight at a time. The STM32 caches the last accepted mutation's ID/signature/result while running. Readback is required after uncertainty, especially across reboot.

Schedule/settings RESULT success follows a real flash save; clock success follows write/tick verification. A failed storage attempt can leave changed values in RAM while later persistence retries run. Do not interpret a failed or timed-out save as a guaranteed rollback.

Manual report success also waits for a real flash commit. Report status 0 is unknown, 1 is user-reported taken and 2 is user-reported not taken. A stale previous report is rejected as a conflict. Reports do not acknowledge the physical alarm.

Activity modes: 0 metadata (45-byte response including history generation), 1 per-slot totals (46 bytes), 2 reminder record (54 bytes), 3 completed interval (26 bytes). Each tagged reply begins with mode, index and uint32 analytics generation. A six-byte request carries the same fields; mismatched generations are not returned. The STATE payload remains 194 bytes. New clients request history with index + uint32 history generation, and receive the original 16 bytes plus that generation; old one-byte requests/16-byte replies remain supported. See [activity definitions and layout](19-activity-dashboard.md).

## HTTP routes

Base URL: **http://192.168.4.1** on the MedBox network. There is no TLS endpoint. Responses are JSON except the dashboard/captive portal routes.

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/` | Self-contained dashboard |
| GET | `/api/state` | Telemetry, schedules, history, per-boot token and latest operation result |
| GET | `/api/network` | AP status, channel, connected stations, page hits and diagnostic counters |
| POST | `/api/slot` | Set one complete reminder slot |
| POST | `/api/settings` | Set opening tone/display settings |
| POST | `/api/clock` | Set/verify the RTC clock |
| POST | `/api/test` | Start repeating test alarm or show existing medicine alarms |
| GET | `/api/analytics` | Activity metadata and cumulative per-medicine totals |
| GET | `/api/doses?offset=0&limit=8` | Up to eight retained reminder measurements per page |
| GET | `/api/intervals` | Eight retained completed alarm intervals |
| POST | `/api/report` | Save an explicit medicine self-report |

POST content type is `application/x-www-form-urlencoded`. Supply **`X-Medbox-Token`** from `/api/state` and the current **`revision`** field. These protect against accidental cross-origin form writes and stale drafts; they are not user authentication. Wi-Fi network access controls access to the page/token.

### Fields

| Route | Required fields in addition to revision |
| --- | --- |
| `/api/slot` | `slot` 0–7; `name` 1–16 letters/numbers/spaces; `hour` 0–23; `minute` 0–59; `days` 1–127; `enabled` 0 or 1 |
| `/api/settings` | `sound` 0/1; `brightness` 1–10; `duration` 100–2000, divisible by 100 |
| `/api/clock` | `epoch` 0–3155759999 in the civil epoch described above |
| `/api/test` | None |
| `/api/report` | `id` retained record ID; `report` 0/1/2; `previous` expected current report 0/1/2 |

Names are trimmed and uppercased by the ESP32. Even disabled slots require at least one selected weekday. `sound=0` disables only the opening information tone; medicine/test alarms remain audible.

A valid queued mutation returns **202** with `{"operation": <id>}`. This is not a saved-success response. Poll `/api/state` until the matching `operation` is no longer `pending`. Require `result=0` and `revision=resultRevision` before reporting a confirmed save. HTTP 400 means invalid input, 403 missing/wrong token, 409 busy/stale state, and 503 stale/offline STM32 telemetry.

The dashboard handles this sequence directly. Keep browser drafts tied to their original revision; silently replacing the revision can overwrite a concurrent edit.

## USB hardware test transport

At 115200 baud the ESP32 USB console accepts lines such as:

```text
GET /api/state
GET /api/network
POST /api/test revision=12345
```

It creates a loopback TCP request to the same HTTP server and adds the local token. Responses are bracketed by `USBHTTP BEGIN` / `USBHTTP END`. This is a local maintenance path for the same UART/validation/save logic; it is not an alternate simulated controller. `tools/check_device_http.py` provides a Python client. Close other USB monitors before using it.
