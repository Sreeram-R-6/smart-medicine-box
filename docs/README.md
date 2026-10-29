# Documentation index

## Current guides

1. [Build, flash and first run](getting-started.md)
2. [Hardware and wiring](hardware.md)
3. [Architecture and persistence](architecture.md)
4. [UART protocol v2 and HTTP API](protocol-and-api.md)
5. [OLED UI 1.1](18-oled-ui.md)
6. [Validation and troubleshooting](validation.md)
7. [Development integration notes](17-local-control-and-validation.md)
8. [Activity dashboard and measurement definitions](19-activity-dashboard.md)

Also see [host tests](../tests/README.md), [contributing](../CONTRIBUTING.md), [local access/security](../SECURITY.md) and [third-party notices](../THIRD_PARTY_NOTICES.md).

## Historical bring-up notes

Documents numbered **01–16** and the original `diagrams/` files preserve the project's wiring investigation and intermediate firmware milestones. They contain superseded statements such as RAM-only schedules, protocol v1, unimplemented OLED menus, conditional pins or a quiet/expiry behavior that is no longer current. Do not follow them as the current installation sequence. The guides above and actual firmware take precedence.

Current firmware labels are STM32 `1.2.0-activity` and ESP32 `1.1.0-activity`. Notes 17/18 preserve the UI/integration milestone; guide 19 describes the new activity measurements. A clock running on USB still does not certify the known RTC backup battery issue.
