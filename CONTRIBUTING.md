# Contributing

Start with [architecture](docs/architecture.md) and [protocol/API](docs/protocol-and-api.md). Keep hardware-independent logic in the existing C modules so it can be verified without a board.

Before submitting a change:

```sh
python tools/test_host.py
node tools/test_dashboard.mjs
python tools/check_project.py
pio run -e stm32
pio run -e esp32
```

If editing `dashboard.html`, regenerate `web_ui.h` with `python tools/embed_dashboard.py`. If changing OLED layout, run the screen tests and `python tools/render_oled_previews.py`, then inspect the actual pixels for maximum names, footer overlap, selected/editing states and clock warnings.

Describe the concrete before/after behavior and relevant checks in the pull request. State explicitly whether a change was tested on hardware. Host tests do not establish electrical safety, battery health or radio behavior.

Preserve these invariants:

- STM32 remains the configuration/reminder authority; web success requires verified acknowledgment/readback.
- Stale edits, timeouts and write failures are not silently presented as successful saves.
- Clock validity/trust remains distinct from backup-battery health.
- A lid event is not labeled as confirmed medicine ingestion.
- Flash format/linker boundaries remain compatible or have an explicit migration/reset plan.
- UART/I²C buffers and operations stay bounded; animation must not stall alarm processing.
- Do not commit real credentials, device memory dumps or personal medicine histories.

No project-wide license for original application code has been selected yet. Retain existing third-party notices and discuss licensing with the owner before assuming a redistribution license.
