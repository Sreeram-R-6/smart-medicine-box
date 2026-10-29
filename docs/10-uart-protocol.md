> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 10 — Diagnostic UART protocol v1

Shared code: shared/protocol.h. Fixed ten-byte binary frame, little-endian integers:

| Offset | Meaning |
|---|---|
| 0–1 | A5 5A sync |
| 2 | Version 01 |
| 3 | 1 HELLO, 2 HEARTBEAT, 3 reserved STATUS, 4 ACK |
| 4–5 | uint16 sequence |
| 6–7 | uint16 value; ACK value is original type |
| 8–9 | CRC16-CCITT-FALSE, init FFFF/poly 1021, over bytes 2–7 |

Each board sends initial HELLO then five-second HEARTBEAT; these receive ACK with original sequence. ACKs are not acknowledged. Link timeout 15 seconds. Parser rejects invalid CRC/version, resumes scanning, expires partial frames after 100 ms when another byte arrives. Example prefix: A5 5A 01 01 01 00 00 00 means HELLO seq=1 value=0, followed by calculated CRC.

Bounds: ten-byte parser, 128-byte STM32 RX ring (127 usable), 1024-byte log ring, one TX frame in flight. Full queues drop/count instead of blocking. Statistics every 30 seconds. No dynamic allocation in STM32 application.

This is diagnostic transport, **not the finished application protocol**. No command retries, durable IDs/deduplication, schedule changes, event journal or applied acknowledgements yet. Repeated heartbeat messages have no medication side effects. ACK means frame receipt, not validation/persistence/application.

Next protocol must specify bounded payloads, sessions, durable event/occurrence IDs, command revisions, finite retries/NACKs and separate received/validated/persisted/applied results. Website success requires STM32 applied response. Never build schedule mutation endpoints around this diagnostic ACK.
