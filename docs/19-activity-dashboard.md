# Reminder activity dashboard

STM32 firmware `1.2.0-activity` and ESP32 `1.1.0-activity` add persistent reminder measurements and manual medicine reports. Refresh the page at http://192.168.4.1/ after updating both controllers.

## What the dashboard means

| Display | Source / definition |
| --- | --- |
| Awaiting opening | A real scheduled reminder is active and has not received a fresh lid opening |
| Box opened | The IR sensor recorded a closed-to-open transition acknowledging that reminder |
| Reported taken / not taken | Explicit user report from the web buttons; never inferred from the sensor |
| First opening after alarm | Trusted RTC wall-clock difference between the scheduled timestamp and first acknowledged opening; RTC sampling has approximately one-second resolution |
| Self-report after alarm | Time between scheduled alarm and the user's manual report, when both clock timestamps are valid; not the exact moment of ingestion |
| Ringing duration | Measured powered controller time while that reminder was active, including the melody's short gaps; not a microphone measurement of actual sound |
| Extra openings | Physical openings that did not acknowledge an armed pending medicine reminder |
| Openings before this alarm | Openings in the preceding alarm-to-alarm interval, including the acknowledgment opening; the extra-opening count is also shown |

Manual reports do not silence the physical alarm. It still stops only after a fresh lid opening. Clear report returns the record to unreported; it does not remove the underlying reminder or lid measurement.

The system cannot detect swallowing or independently confirm medicine ingestion. A late self-report records when the user reported, not the exact moment they took medicine. Imported, interrupted and uncertain records are labeled.

## Retention and averages

The STM32 retains the **latest 24 per-medicine reminder records**, **eight completed alarm intervals**, and cumulative per-slot counters/timing sums. When old detail rotates out, cumulative counts and averages continue. Record names are captured when the reminder is created; per-slot aggregate rows use the current slot name and are labeled accordingly.

Average opening delay uses only records with a trustworthy measured RTC difference. Average ringing uses completed, uninterrupted, non-imported reminders. Pending alarms and standalone BACK+OK test alarms are excluded from these averages. Simultaneous medicines are separate records sharing one physical alarm; their timing contributes once per medicine, not once per buzzer session.

Ringing after a reboot is a lower bound: prior checkpoints plus newly powered time, excluding time switched off. Such interrupted sessions are excluded from complete-session ringing averages. Active ringing checkpoints are coalesced to once per minute, while alarm transitions and report changes request immediate commits. A sudden power failure can lose a recent uncommitted checkpoint or ordinary lid count. A successful report is acknowledged only after a flash commit and readback.

No numerical averages are fabricated when there are no samples. The page shows Not measured / Updating / Waiting as appropriate. Partial intervals are flagged after a restart, uncertain clock period or before the first complete alarm boundary.

## Interface

- Summary: tracked reminders, manual taken reports, average first opening and average ringing.
- Reminder log: original medicine name/time, lid status, report status, timing, preceding opening count and three report controls.
- Filters: pending, unreported, taken/not-taken reports and medicine slot. Six reminders are displayed per page.
- Per-medicine totals: lifetime counts and means, plus quantitative opening-delay bars.
- Box activity: current interval, cumulative openings, extra openings and eight completed intervals.
- CSV export: downloaded locally from the records currently loaded on the phone. It includes sensor/report distinctions and uncertainty flags. It is not sent to a cloud service.

The existing schedule, clock, sound, OLED and test-alarm controls remain available. The UI supports a narrow phone layout and the operating system's light/dark theme. Controls are disabled for stale/offline data and during pending writes.

## Persistence upgrade

Flash schema **2** appends analytics after the unchanged 2072-byte schema-1 prefix. The new payload is 3760 bytes and fits with its 24-byte header inside each existing 4096-byte bank. Startup accepts a valid schema-1 bank, preserves schedules/settings/history/pending alarms and starts analytics without inventing measurements from old events. The next commit writes schema 2 to the inactive bank.

Pending pre-upgrade alarms may be imported with their known names/due times, but earlier ringing/response measurements remain unknown and are excluded from complete averages. Back up storage before an upgrade. Old firmware that only understands schema 1 cannot read schema-2 banks; downgrading requires an explicit restore/migration plan.

## API additions

| Route | Meaning |
| --- | --- |
| GET `/api/analytics` | Tracking metadata, current opening interval, retention counts and eight slot summaries |
| GET `/api/doses?offset=0&limit=8` | A page of up to eight records; offset 0–23 |
| GET `/api/intervals` | The latest eight completed intervals |
| POST `/api/report` | `id`, `report` (0 unknown, 1 taken, 2 not taken), `previous`, and current config `revision`, with the normal token header |

Reports use the same transaction/retry/flash-confirmation path as schedule changes. `previous` detects conflicting reports from another client. The browser waits for both acknowledgment and matching record readback.

UART adds type **25 CT_ANALYTICS** and **26 CT_REPORT**, preserving the existing 194-byte STATE payload. Analytics request payloads are empty for metadata, or mode/index/generation for summaries and records. Generation tags prevent mixing rotated records during transfer. The history request/response also supports a generation tag to avoid mixing ring indices while new events arrive. Existing basic state/control messages remain compatible; both boards need the update for activity features.

## Verification

Host tests cover event grouping/deduplication, timing, interval counts, manual-report conflicts, restoration/clock uncertainty, bounded retention, packet generations and schema-1 decode. Dashboard tests check measured averages, separate lid/report states, filtering and rejection of mixed generations. Hardware checks and observations are recorded separately from host tests; a test fixture is not a real medicine result.

On attached hardware, a clearly named ACTIVITY TEST reminder recorded a physical opening 13 seconds after its scheduled time and 12,295 ms of active ringing. Taken/not-taken/clear transitions committed and read back. The temporary slot was restored and its report cleared. One subsequent extra opening produced two total openings and one extra. A real STM32 restart retained the measured record, cumulative timings/counts, original schedules and settings; both schema-2 banks had valid CRCs. The user confirmed these values and the new UI on the phone. The test record remains visibly labeled as a test, not a real medicine report.

UI review: Operate mode, variance 3 / motion 1 / density 6; the existing forest-green identity is retained with readable light/dark themes. Metrics use sample-aware empty states, filters, bounded pagination and explicit coverage flags. Focus is preserved across data refreshes and pointer presses are not replaced mid-click. The design scanner reported zero hard violations and zero warnings. No external fonts, scripts or chart libraries are loaded.

| Before | After | Why |
| --- | --- | --- |
| A generic device-event list | A per-medicine measured reminder log | Connect each alarm to its actual response |
| No explicit medicine report | Separate manual taken/not-taken state | Avoid inferring ingestion from the sensor |
| No ringing/opening statistics | Sample-aware means and interval counters | Show useful measurements without invented zeros |
| Rebuilt live controls on every clock tick | Change-driven rendering with focus/press preservation | Keep phone and keyboard interaction stable |
