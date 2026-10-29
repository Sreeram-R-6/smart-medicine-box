# OLED UI 1.1

The 128x64 OLED now uses a pixel renderer with a large clock, full-width medicine names, generous list spacing and clear selection bars. Physical navigation remains LEFT/RIGHT, OK and BACK; BACK+OK still starts the repeating test alarm.

## Screens

- Boot: a capsule mark and MEDBOX title reveal, followed by a short underline sweep. About 2.2 seconds after the display initializes. Button input can skip it; a pending alarm takes priority. UART, RTC, input scanning and reminder processing continue during the animation. There are no blocking animation delays or fake health-check percentages.
- Home: current HH:MM in double-size type, seconds, weekday/date, lid state, next alarm time and day, full medicine name, and clock health. Simultaneous reminders cycle their full names every three seconds. Invalid/untrusted time shows a clear clock warning instead of a misleading next alarm.
- Main menu: four clearly separated choices, inverse selection and stable button hints.
- Schedules: one complete reminder per page, full 16-character name, large time, enabled state, weekdays and slot indicators. LEFT/RIGHT changes slots; OK edits.
- Clock, alarm editor, weekdays and settings: three spaced rows with a position counter and scrollbar; edited values have an underline. Selecting the name opens the full-width character editor.
- History: full event title, large timestamp and date. Untrusted historical time is labeled explicitly.
- Alarms: prominent due time and full medicine name; the ten-second repeating melody and fresh lid-opening acknowledgment remain unchanged.

The next-reminder calculation checks enabled slots against weekdays, includes tomorrow and later-week occurrences, groups equal timestamps, and respects the supported 2000–2099 calendar. Disabled alarms are excluded. It reads the same saved schedules used by actual reminders.

## Design review

Operate mode: variance 3, motion 3, density 6. The design follows a monochrome instrument display: fixed hardware theme, pixel typography, strong hierarchy and minimal decoration. The existing four-button interaction model is retained.

| Before | After | Why |
| --- | --- | --- |
| Eight equally weighted text rows | Large clock and a distinct next-reminder panel | Important information reads at a glance |
| Names truncated in schedule lists | Full-width 16-character name and large time | Users can identify the actual reminder |
| Five small editor rows | Three spaced rows, selection bar and position indicator | Easier navigation on a small screen |
| Tight footer labels and setting values | Shorter hints and wider value spacing | Prevents adjacent labels merging visually |
| Technical save-success wording | READY TO USE / TIME UPDATED | Clear product language |

Pre-flight: 23 exact C-rendered framebuffer previews were reviewed and refined. Text, values, footer spacing, selection contrast, maximum names, disabled/empty states and clock warnings were checked. Guarded rendering tests cover every view and edit/selection combinations. The fixed monochrome hardware has no hover, browser focus, responsive layout, external assets, web vitals or alternate theme; those web-specific checks are not applicable. Boot motion is brief and skippable; normal menu changes are immediate, and medicine names change without scrolling. No UI data is fabricated in normal operation.

## Verification

- STM32 firmware built and flashed with readback verification; 34,480 bytes of 57,344 application flash and 10,244 bytes of 20,480 static RAM used.
- Calendar/next-reminder tests passed for midnight, weekdays, an occurrence seven days away, equal-time groups, disabled schedules, invalid dates and the end of 2099.
- Guard bytes verified all framebuffer writes remain within 128x64 storage.
- Existing menu navigation, reminder/RTC/storage and UART validation tests passed.
- User confirmed the boot animation and new screens look good and menu navigation works.
- Actual bytes sent to the OLED were captured: Home showed 20:09, NEXT 23:00 TODAY, REMINDER 6 and CLOCK OK, matching live saved schedules.
- ESP32 remained connected with zero reported UART frame, RTC read and OLED write errors during the final checks.

Framebuffer previews: `.pio/oled-ui-preview.png`; actual screen capture: `.pio/ui-home-live.png`. Generate previews by building/running `tests/screen_test.c`, then running `python tools/render_oled_previews.py`. These are literal firmware bitmaps, not design mockups.

The known depleted RTC battery is a separate hardware issue. A running-clock check on USB power does not certify time retention during power loss.
