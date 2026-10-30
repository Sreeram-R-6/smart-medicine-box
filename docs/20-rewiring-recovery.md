# Rewiring recovery and current bench status

## Firmware 1.2.1-recovery

After the assembly was rewired, both the RTC and OLED transfers failed with the prior image. The original image was read back and matched the rebuilt binary exactly; this was not an accidental firmware overwrite. The STM32 supply measured approximately 3.29 V at the programmer, which does **not** measure the supply at either module.

The shared B6/B7 bus now uses a single-master GPIO open-drain transport instead of the F1 hardware I2C engine. At the configured 8 MHz core clock, each half-period waits at least 40 microseconds, so the maximum nominal clock is 12.5 kHz; interrupts and GPIO overhead can slow it further. SCL release is checked for clock stretching with a finite 200-half-period bound. Recovery sends up to nine clock pulses and a STOP, with both lines released on exit. Error codes are 1=NACK, 2=SCL held low, 3=SDA held low.

This avoids dependence on the peripheral START/BUSY behavior. ST documents relevant F1 hardware limitations in [ES096](https://www.st.com/resource/en/errata_sheet/es096-stm32f101x8b-stm32f102x8b-and-stm32f103x8b-mediumdensity-device-limitations-stmicroelectronics.pdf); that is background, not proof that a particular observed failure is caused by silicon errata.

OLED initialization sends command/argument pairs separately. Each framebuffer chunk restores its page and column before sending data. If a write accepts only part of a chunk, the next attempt restores the original position before retransmitting; the shadow buffer advances only after success. Three successive failed frame transfers trigger complete initialization and redraw. Existing two-second reconnect retries remain.

## Observed results, 2026-10-30

- STM32 programming and readback verification passed. Persistent storage was backed up locally before programming; saved schedules were retained.
- All eight host C suites passed, including transport ACK/NACK, repeated-start register reads, stretch/stuck-line bounds, OLED partial-write retry, shadow integrity and reconnect tests.
- ESP32 USB HTTP diagnostics reported a configured MedBox AP and fresh STM32 UART telemetry, with no invalid UART frames in the observed samples.
- Physical button and lid events reached the firmware; the BACK+OK combination activated its test alarm. Firmware activity does not verify audible sound.
- The RTC accepted a current India-time synchronization. A later capture showed trusted, advancing time with OSF clear and zero read failures in that run. One subsequent sample counted a transient read failure and recovered trust.
- **OLED acceptance is still failing on the rewired assembly.** Address/data acknowledgments are intermittent, and the latest captured run had no completed new framebuffer. A lit panel or retained Home image is not proof of fresh updates. Module supply, jumper continuity, pull-ups, connector contacts and the display itself still need electrical inspection. Firmware changes do not certify those connections.
- A final set of three warm resets retained an advancing RTC calendar and had zero RTC read errors in each short sample, but OSF returned and the clock was not trusted. The OLED failed initialization on all three resets. Thus the successful powered synchronization does not establish dependable boot-time clock trust either. Saved configuration checksum remained unchanged.

Earlier successful OLED demonstrations remain historical evidence for the previous assembly. They must not be treated as a pass for the current wiring. No claim of reliable every-boot operation is made while these failures remain.

## Recovery checks

1. Switch off **both** STM32 and ESP32, plus module supplies, before changing wires or performing a cold-start test. An active UART peer can feed an otherwise unpowered board through its signal input. Keep programmer power outputs disconnected as in the wiring guide.
2. Check module labels: OLED/RTC VCC to the verified 3.3 V supply, GND to common ground, SCL to B6 and SDA to B7. Do not rely on connector order across different modules.
3. Measure VCC to GND directly at each module while operating. Check released SDA/SCL levels and continuity with power off. Inspect split breadboard rails and use short, known-good jumpers.
4. If the fault persists, power off and test the OLED alone on the bus, then reconnect the RTC after another power-off. A change in behavior helps isolate the branch; it does not identify the failed component by itself.
5. Confirm changing seconds, responsive menus, stable lid state and audible test melody, then repeat warm resets and complete cold starts. Keep failed samples in the results rather than counting a single successful boot as acceptance.

The known depleted backup battery is separate from USB-powered operation. With no working backup clock or external time source, firmware cannot determine how long the box was switched off. Do not replace an invalid clock with a fixed build timestamp or silently clear OSF on every boot.
