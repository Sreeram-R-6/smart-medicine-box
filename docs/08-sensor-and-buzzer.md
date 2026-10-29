> **Historical bring-up note.** This records an earlier firmware milestone and may contain superseded instructions. Use the [current documentation](README.md), [wiring](hardware.md), and [protocol v2](protocol-and-api.md) for the published release.

# 08 — FC-51 calibration and buzzer safety

FC-51 observes reflected IR, not a mechanical switch. Aim it at a fixed lid target with repeatable surface/distance; shield from ambient light and moving pills. Adjust sensitivity midway between reliable open/closed states.

After measurements: verified supply → VCC; common GND → GND; OUT → PB12 only if safe for 3.3 V GPIO. If operation needs 5 V and OUT reaches 5 V, stop and select translation from its actual output circuit. Do not guess a divider for an unidentified open-collector circuit.

Firmware logs stable raw=0/1 after 100 ms. PB12 has a weak internal 3.3 V pull-up to support an open-collector OUT. Firmware 0.4.0 displays OBJECT NEAR (LOW) or PATH CLEAR (HIGH) using the nominal active-low FC-51 output. A filled/empty bar represents only detection, not centimetres or proportional distance. The screen includes RAW, CHG (filtered transitions), RTC time/status and INPUT NOT YET TESTED until the first transition. The external LED heartbeat is disabled; indication is on the OLED.

The lid's open/closed mapping remains unverified (SENSOR_VERIFIED=0). Previous physical tests showed LOW in both positions while the sensor's own detection LED changed. The proximity display interprets the received input; it does not establish that the module-to-PB12 connection is working. If RAW stays 0, power off, disconnect OUT from B12, then power on: the internal pull-up should yield RAW 1. Check jumper continuity, actual B12 pin, common GND and module OUT if it does. Display updates are requested immediately on a filtered transition without waiting for the next RTC read.

Test twenty open/close cycles in varied lighting. Record polarity. It deliberately does not call transitions dose acknowledgements. A disconnected/stuck digital output can mimic normal state; complete fault detection is impossible from this input alone. The SWD-readable sensor_diag records raw/stable level, transition count, low/high transitions, and last transition uptime.

Leave MH-FMD physically disconnected until supply/current/input circuit are verified. PB0 initializes HIGH, the stated active-low off level. During reset PB0 is high impedance: software initialization alone cannot guarantee silence. A compatible input may need external off bias; an input pulled to 5 V may need a transistor interface. Check onboard driver, input thresholds and startup behaviour first. No buzzer sound test is implemented yet.

Planned nonblocking reminder pattern: 100 ms pulse, 150 ms gap, 100 ms pulse, quiet until next five-second cycle. Silent on startup/idle/fault/network loss. Final driver must default OFF even while STM32 is reset or absent.


## Lid information buzz, firmware 0.6.0

The user confirmed the connected buzzer works and is verified. Firmware now drives the active-low buzzer on PB0 for a nominal 120 ms on each filtered closed-to-open transition (PB12 LOW to HIGH). Timing is polled without a blocking buzzer delay; I2C operations may slightly extend a pulse. The initial sensor reading at boot never triggers a chirp, keeping an already-open box silent at startup. Holding the lid open does not repeat the sound; another stable close/open cycle triggers a new chirp. No reminder schedule is implemented by this sound.

The OLED now shows BOX: OPEN / BOX: CLOSED, using target detected LOW as closed and HIGH as open. SENSOR_OPEN_LEVEL controls that mapping. buzzer_diag exposes active/start/count/completed through SWD for verification. Earlier paragraphs describe the investigation before the sensor and buzzer were connected and tested.


The first physical chirp test produced a response but no audible tone, and the user reported a voltage drop. ENABLE_OPEN_CHIRP is currently 0 pending confirmation of whether the 3.3 V supply rail actually sags or only the active-low I/O signal falls. Do not lengthen the pulse or change module supply voltage to mask a rail problem. Module active/passive type and actual VCC/GND/I/O connections are still being checked. The automatic chirp implementation above remains present but disabled.


## Audio-frequency drive, firmware 0.6.1

The steady-low 500 ms pulse produced no audible tone despite five completed firmware triggers and no observed controller reset. Firmware now uses TIM3 channel 3 on PB0 to output a 2 kHz, 50% duty waveform during the same 500 ms opening event. Between events CCR3=ARR+1 holds the active-low module input HIGH. Hardware timing uses the 8 MHz HSI timer clock, PSC=7 and ARR=499. ENABLE_OPEN_CHIRP is 1 again, following the user's explicit request to enable it. A passive module requires an alternating waveform; the exact module type and audible result still require physical confirmation. Earlier disabled/steady-drive notes describe prior diagnostic versions.
