> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# OLED menu, firmware 0.8.0

The existing 128x64 display now provides Home, Set Clock, Schedules, History and Settings. The user confirmed navigation through all menu sections and the opening tone on the board.

## Buttons

| Button | Pin | Browse | Edit |
|---|---|---|---|
| LEFT | PA0 / A0 | Previous item | Decrease value / previous letter |
| RIGHT | PA1 / A1 | Next item | Increase value / next letter |
| BACK | PA4 / A4 | Parent screen; main menu returns Home | Cancel the current value |
| OK | PA5 / A5 | Open selected item | Accept value / select Save |

Each switch connects its input to GND when pressed. Internal pull-ups are enabled, with 30 ms debounce. Held LEFT/RIGHT repeat only during value editing, after 500 ms and then every 140 ms. OK/BACK do not repeat. Chords are ignored for navigation. The selected row is inverted; the header is also inverted. Every screen fits eight rows of 21 characters.

## Home

Shows RTC time/date, lid state, clock trust, and selected schedule-slot count. Press OK for the main menu. The opening tone continues independently of the current page. Clock warnings are displayed rather than silently cleared.

## Set Clock

Select Year, Month, Day, Hour, Minute or Second with LEFT/RIGHT. OK starts editing; LEFT/RIGHT changes the value; OK accepts the draft value. BACK during editing restores the previous value. Month/year changes clamp the day to the month's actual length, including leap years.

Scroll to SAVE TO RTC and press OK to write the draft to the DS3231. This validates the calendar, enables the oscillator, verifies readback and clears OSF. The result screen reports success only after the hardware operation succeeds. A failed write preserves the draft for retry. BACK while browsing fields cancels the whole clock draft. The clock is never overwritten automatically at boot.

## Schedules

Eight slots support a 16-character name, hour, minute, weekday mask and selected ON/OFF flag. Choose a slot with LEFT/RIGHT and press OK to edit. Select SAVE SLOT IN RAM to commit the draft. BACK from the field list discards that slot's unsaved changes.

For names, LEFT/RIGHT moves between the 16 character positions and DONE. OK on a character edits it; LEFT/RIGHT cycles SPACE, A-Z, 0-9; OK accepts that letter. BACK cancels a letter edit, or cancels the name when browsing positions. Select DONE to return to the slot editor without stepping through every character.

For weekdays, LEFT/RIGHT selects Monday through Sunday or DONE; OK toggles a day or accepts DONE. BACK discards weekday changes. Saving requires a nonblank name and at least one weekday.

**Schedule slots are setup data only. This firmware does not run scheduled reminders or acknowledge doses. Slots reset to defaults after restart.** The list and Home screen state these limits explicitly.

## History

Stores the latest 32 actual device events: startup, lid opening/closing, successful menu clock settings, schedule edits and settings edits. LEFT/RIGHT selects an event; OK opens details; BACK returns. An opened event detail remains stable while new events arrive. Oldest entries are overwritten when full.

Events use RTC date/time only when the clock is valid and trusted. Otherwise they display uptime. Lid events never claim a medicine was swallowed. History is RAM only and resets at restart. SWD clock synchronization is a separate diagnostic path; it is not recorded as a menu action.

## Settings

- OPEN TONE: toggle the automatic lid-opening tone.
- TONE LENGTH: 100-2000 ms in 100 ms steps, default 500 ms.
- BRIGHTNESS: levels 1-10, default 5; applied after accepting the value.
- TEST TONE: explicitly play a tone, including when automatic opening tones are muted.
- DIAGNOSTICS: button mask, lid state, OLED/RTC error counts and uptime.

Settings are RAM only. BACK cancels a value edit; pressing BACK again returns to the main menu. Tone duration is captured at the start of a pulse so a setting change cannot extend an already-playing tone.

## Implementation and validation

- `firmware/stm32/menu.c` and `menu.h`: hardware-independent state machine, drafts, history and text rendering.
- `main.c`: debounced button dispatch, RTC save handshake, runtime sound/contrast and display integration.
- `oled_test.h`: framebuffer row comparison sends only changed pages; recovery forces a full redraw.
- `tests/menu_test.c`: calendar boundaries, navigation, cancellation, invalid drafts, clock save failures, name/day editing, settings, history wrap/detail stability and 100000 navigation/render iterations.

Host tests passed with `-std=c11 -Wall -Wextra -Werror`. STM32 build and flash verification passed. Board inspection showed all four buttons being detected, advancing OLED frames, and zero OLED/RTC communication errors or CPU faults. The user confirmed all menu pages and the opening tone work. Clock battery retention, reminder logic and permanent storage remain separate work.

Run host checks from the project root:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Ifirmware/stm32 firmware/stm32/menu.c tests/menu_test.c -o .pio/menu_test.exe
./.pio/menu_test.exe
```
