> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# OLED menu design verification

Reading this as: a four-button embedded control menu for the medicine-box owner, using the existing monochrome OLED and bitmap font. Operate mode; variance 2, motion 1, density 8. The fixed eight-row display requires high density and instant feedback. Black/white is a hardware constraint; no decorative animation is used.

| Before | After | Why |
|---|---|---|
| Diagnostic-only button names | Highlighted navigation, drafts and explicit Save | The same four buttons now control the device |
| Full-frame redraw on each interaction | Only changed rows are transmitted | Less I2C traffic and quicker selection feedback |
| Name entry required stepping through all characters | Cursor selection plus DONE | Short names are practical to edit |
| Long editing-state label | BACK CANCEL VALUE | Fits the 21-column limit and explains cancellation |
| Live history detail could change when new events arrived | Snapshot the selected event | Keeps the detail being read stable |

## Pre-flight

- [x] Brief, Operate mode and dials recorded above.
- [x] Existing display/font/button identity retained; no new visual system.
- [x] No em-dash or unsupported glyphs in product strings.
- [x] One fixed monochrome theme; full-row inverse selection used consistently.
- [x] Full-contrast text and character highlight remain readable.
- [x] Typography uses the existing 5x7 font in an 8-pixel row; strings fit 21 columns.
- [x] Visible copy reviewed in rendered previews of twelve screens and a board framebuffer capture.
- [x] Navigation is instant; no animation or scrolling motion.
- [x] Single theme justified by physical monochrome OLED.
- [x] Responsive layout: N/A, fixed hardware display.
- [x] Focus, edit, busy, empty, error, save success and cancelled states implemented; hover N/A, physical buttons.
- [x] Rows and spacing replace decorative cards; consistent inverted headers/selection.
- [x] Images/decorative icons: N/A, text menu.
- [x] Clock, counters and history use real data; setup-only schedules and RAM lifetimes are stated.
- [x] No marketing copy or fabricated outcomes.
- [x] Core Web Vitals: N/A, embedded firmware. Changed-row updates reduce display latency.
- [x] Operate: title, selected control and contextual button actions visible; long lists scroll logically.
- [x] Operate: destructive persistent actions absent; BACK cancels drafts; Save is explicit.
- [x] Operate: useful empty/error states; failed RTC save keeps the draft for retry.
- [x] Operate: no placeholder medicine outcomes; schedule defaults are clearly inactive.

The skill's Node-based CLI/preflight could not run because Node is not installed in the shell. Equivalent source/string review and actual bitmap rendering were performed; browser/CSS checks are not applicable to this hardware surface. Local artifacts are `.pio/menu-preview.png`, `.pio/menu-board.png` and `.pio/menu-board-validation.txt`.
