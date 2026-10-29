> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 12 — Remaining work and validation

## Next STM32 milestone

Full calendar validation/set workflow, identified OLED driver/font, four-button menu, independent device/menu/sensor/link states. Device states: BOOT, IDLE, REMINDING, ACKNOWLEDGED, MISSED, FAULT. Eight weekday schedules, immutable active snapshots, recent history and persistence.

Occurrence key: date + schedule ID + scheduled minute. Same-time medicines form an explicit displayed group. A fresh close/open can acknowledge that group only. Different due times remain distinct even with overlapping windows; select a group and require a new close/open per group. Record sensor/manual sources separately. Menu OK is never implicitly medication taken.

RTC calendar drives occurrence creation; monotonic time drives active windows. An already-open lid cannot acknowledge. Repeated/late openings cannot rewrite outcomes. Invalid RTC suspends new reminders. Forward/backward clock changes reconcile skipped/known occurrences without duplicate triggering. Reset reconciles persisted active checkpoints using valid RTC and reports downtime uncertainty, rather than pretending monotonic time survived. Never suggest extra medication after a miss.

Reserve storage with an explicit reduced linker program region after measuring code size. Plan recoverable versioned settings plus bounded circular history journal with CRC/commit marker, limited writes and power-cut recovery. Target 64 events, overwrite oldest and count losses. No storage is reserved/written yet. STM32 remains authoritative; ESP32 mirrors cannot silently replace its settings.

## Next bridge and website milestone

Reliable bounded application protocol, durable offline event queue/deduplication and replay, explicit persistence ACK, then backend adapter. Website remains a separate scope gate: local ESP32 hosting for LAN use versus authenticated hosted frontend/backend with outbound encrypted ESP32 connections for remote access. A hosted browser cannot reach a private home IP directly.

Proposed website separates browser/backend, ESP/backend, ESP/Wi-Fi, ESP/STM32 and RTC validity; shows last seen/stale data; edits stay pending until STM32 applied ACK; revisions prevent stale concurrent edits. Asia/Kolkata initially; document conversion between RTC local time and backend UTC. Authentication, device ownership, validated TLS and secrets outside frontend required. No backend selected or implemented yet.

## Validation

No hardware connected/flashed by this agent. Electrical and unidentified-module checks remain open. Build results are recorded below after compilation. Compilation does not verify wiring or module compatibility.

Builds on 2026-10-28: `pio run -e stm32 -e esp32` passed for both targets.

| Target | RAM | Flash/application size |
|---|---|---|
| STM32 | 1,628 / 20,480 bytes | 7,732 / 65,536 bytes |
| ESP32 | 37,860 / 327,680 bytes reported by build profile | 727,592 / 1,310,720 bytes application partition |

ESP32 numbers describe the selected build profile, not independently measured SuperMini memory. Resolved packages: STM32CubeF1 1.8.7, GCC ARM 7.2.1; Arduino-ESP32 2.0.17 (PlatformIO package 3.20017.241212), RISC-V GCC 8.4.0. The initial build exposed a sequence-order warning and an ESP32 min-template type mismatch; both were corrected before the successful build.

Host shared-parser tests passed: CRC known vector, corruption rejection, framing, timeout, noise resynchronization, unsupported version and uptime wrap. See tests/README.md.
