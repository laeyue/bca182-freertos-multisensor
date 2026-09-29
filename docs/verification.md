# Wokwi functional verification record

Build with `pio run -e bluepill_f103c8`, start Wokwi for VS Code, and record the **observed** OLED/serial/buzzer behavior before assigning PASS or FAIL. Native tests and static analysis do not exercise Wokwi peripherals, so interactive observations are recorded separately below.

## Startup wakeup diagnosis

The original Wokwi run reached `Scheduler running` and
`Sensor: sampling DHT22`, then omitted the DHT start-delay completion and
MotionTask heartbeat. Those messages precede DHT bit decoding, so sensor data
alone could not explain that stall. After the SysTick and PendSV port changes,
a clean Wokwi run reached `Sensor: DHT start delay finished`,
`Sensor: DHT22 read complete`, repeated sensor samples, and MotionTask
heartbeats. One manual restart attempt did not show progress; a rebuild/reload
then produced repeated wakeups. The latest clean image also showed continued
sampling after reload. The startup symptom is considered cleared in the
observed reload run, though restart behavior should be monitored in later runs.

On 2026-09-30, a follow-up live run in VS Code Wokwi again progressed beyond
the earlier stall. The visible serial output included `Sensor: sampling DHT22`,
`Sensor: DHT start delay finished`, `Sensor: DHT22 read complete`,
`Sensor: 33.1 C, 66.0 %`, `Sensor: light 76 % ADC 1001`, and recurring
`Motion: heartbeat PIR high` messages. While ACTIVE, the OLED showed the
clockwise page sequence Temperature -> Humidity -> Light -> Motion ->
Temperature and the reverse sequence Temperature -> Motion -> Light ->
Humidity -> Temperature. `Input: display page changed` appeared as the pages
advanced. At 33.1 C the Temperature page displayed `ALARM`, and the Wokwi
buzzer activity icon was visible. A temporary test setup used DHT22 33.1 C /
66.0% and a 86,400-second PIR hold because Wokwi fast-forwarded simulated time
between manual interactions. The committed diagram was restored to 24 C / 40%
and the normal five-second PIR hold after the run. This live observation was
not saved as a new image; the earlier retained production-run screenshots and
the new text run record are linked below. No independent physical acoustic
measurement was made.

### Static ELF audit

The production ELF places the vector table at `0x08000000`. Its SVC, PendSV,
and SysTick entries resolve to `SVC_Handler` (`0x08001810`),
`PendSV_Handler` (`0x08001870`), and `SysTick_Handler` (`0x08003ef8`). The
disassembly also shows `vTaskDelay()` setting `PENDSVSET`,
`xTaskIncrementTick()` incrementing the tick and moving expired tasks from the
delayed list to the ready list, and PendSV calling `vTaskSwitchContext()`.
These checks rule out a missing or weak exception-vector mapping in the linked
image. They do not show whether PendSV runs in Wokwi or whether UART output is
being lost after the delay.

## DHT22 bus and timing fixes

The failed read was waiting for the sensor's response-high edge. PB12 stayed in
output open-drain mode after the host released the bus, so the STM32F1 internal
pull-up was not enabled. The [AM2302 datasheet](https://files.seeedstudio.com/wiki/Grove-Temperature_and_Humidity_Sensor_Pro/res/AM2302-EN.pdf)
recommends an approximately 5.1 kΩ external pull-up and releasing the bus by
switching the MCU pin to input. The diagram now includes that resistor, and the
driver switches PB12 to input-pull-up for receive and waits two seconds after
power-up before the first sample.

The initial receive implementation classified bits by measuring the high pulse
between two polled GPIO edges and testing whether the result exceeded 50 us.
Because both edge timestamps include polling delay, that decision could move
near the zero/one boundary and produce bad bytes/checksums. The revised decoder
samples the bus 40 us after the rising edge, between the AM2302's short zero
pulse (26-28 us) and long one pulse (about 70 us), then waits for the falling
edge before starting the next bit. A live VS Code Wokwi run produced repeated
checksum-valid `24.0 C, 40.0 %` and `32.0 C, 65.0 %` readings after changing the
DHT controls. No checksum-failure diagnostic appeared in the observed samples.
The timing is based on the [Aosong AM2302 technical manual](https://www.aosong.com/uploadfiles/2025/04/20250417105409216.pdf).

## Buzzer output fix and verification

The high-temperature test initially showed that TIM4 was running while PB8
produced no PWM. The PB8 alternate-function push-pull setup left
`GPIO_InitTypeDef.Speed` at zero; on the STM32F1 this selects the input mode,
so the timer output never reached the pin. `boardInit()` now configures PB8 at
`GPIO_SPEED_FREQ_HIGH` before starting TIM4 channel 3.

With the production firmware, the DHT22 was set to 32.0 C / 65.0%. The live
serial monitor showed that checksum-valid reading, and the logic-analyzer
capture at PB8 contains 4,096 transitions. The measured mean period is about
2.000 ms (approximately 500 Hz) with roughly 50% duty cycle. This proves the
MCU output is producing the configured alarm waveform; it does not constitute
an independent acoustic measurement. The Wokwi buzzer is connected to PB8.

For recovery, the DHT22 was changed live to 23.5 C / 65.0%. The next valid
sample appeared in the serial log, and the recovery VCD ends with PB8 low and
remaining low after the normal-temperature sample. The screenshot and VCD are
stored as `evidence/wokwi-buzzer-recovery.png` and
`evidence/wokwi-buzzer-recovery.vcd`. The no-PWM screenshot records the
pre-fix symptom.

| ID | Stimulus | Expected result | Actual observation | Result |
| --- | --- | --- | --- | --- |
| FT-01 | Set DHT22 to 28 C, select Temperature | OLED shows about 28.0 C within 2 s | DHT control was set to 27.8 C; the Temperature page showed 27.8 C and ACTIVE | PASS |
| FT-02 | Set DHT22 to 65%, select Humidity | OLED shows about 65.0% within 2 s | Humidity page showed 65.0% and ACTIVE; serial output also reported 65.0% | PASS |
| FT-03 | Change LDR lux control, select Light | Relative brightness percentage changes | Light page rendered 97%; serial output changed from 76% (ADC 980 at 501 lux) to 97% (ADC 130 at 13,183 lux) | PASS |
| FT-04 | Rotate encoder clockwise | Temperature -> Humidity -> Light -> Motion -> Temperature | During ACTIVE, the live run showed each page in order and returned to Temperature after the wrap; `Input: display page changed` appeared and the encoder produced CLK/DT pulses | PASS |
| FT-05 | Rotate encoder counterclockwise | Reverse sequence with wraparound | During ACTIVE, the live run showed Temperature -> Motion -> Light -> Humidity -> Temperature, including the reverse wrap; page-change logs appeared | PASS |
| FT-06 | Set DHT22 above 30 C | Buzzer sounds, OLED alarm indicator appears | At 33.1 C / 66.0%, a valid sample was logged and the OLED showed `ALARM`; the Wokwi buzzer activity icon was visible. The retained PB8 VCD measures about 500 Hz at 50% duty at 32 C. Independent physical acoustic level was not measured | PASS (Wokwi) |
| FT-07 | Return DHT22 to 24 C | Buzzer stops within next sensor sample | After changing the live DHT22 to 23.5 C / 65.0%, the next valid sample appeared and the recovery VCD showed PB8 low afterward | PASS |
| FT-08 | Trigger PIR | State log shows ACTIVE and OLED on | During Wokwi motion input, USART1 showed `Motion: detected` and `State: ACTIVE`; the OLED was lit | PASS |
| FT-09 | Wait 15 s after PIR output returns low | State log shows INACTIVE and OLED blanks | `State: INACTIVE` was observed after inactivity, with the OLED blank | PASS |
| FT-10 | Trigger PIR while INACTIVE | OLED restores and state log shows ACTIVE | After inactivity, PIR activation produced `Motion: detected` and `State: ACTIVE`; the OLED returned to its active display | PASS |

## Deliberate FreeRTOS fault experiments

These controlled faults were applied as reversible edits, built and run in Wokwi, then removed. The shipping firmware retains its periodic blocking and UART mutex. Results below describe observed behavior, not predictions.

| Experiment | Temporary change | What to observe | Actual observation | Status |
| --- | --- | --- | --- | --- |
| Remove blocking | Remove `vTaskDelayUntil()` in `MotionTask` | CPU load, lower-priority display/sensor latency, starvation | MotionTask heartbeats repeated continuously; the Wokwi speed indicator fell to 61-67%, and no sensor sample appeared during the observed interval | Observed |
| Raise priority | Set MotionTask priority to 4 and reduce its delay to 1 ms | Scheduling responsiveness of other tasks | The run reached 20 simulated seconds with recurring MotionTask heartbeats but no sensor sample; the simulator ran at about 90-95% speed | Observed starvation |
| Remove mutex | Bypass `serialMutex` in `boardLog` while MotionTask and SensorTask log | Interleaved UART lines | No garbled or interleaved line was visible in the short Wokwi capture; SensorTask and MotionTask messages remained complete. The race was not reproduced at this log rate | Inconclusive |

## Evidence record

1. [Full circuit](evidence/wokwi-full-circuit.png), [final production run](evidence/wokwi-production-final.png), [Temperature](evidence/wokwi-oled-28c-temperature.png), [Humidity](evidence/wokwi-oled-humidity.png), [Light](evidence/wokwi-oled-light.png), and [Motion](evidence/wokwi-oled-motion.png) screenshots record the circuit, stable task wakeups, and page rendering.
2. [Encoder trace](evidence/wokwi-input-components.vcd) contains PA1/PA2 quadrature changes in both directions. [Active encoder screenshot](evidence/wokwi-input-components.png) shows the `Input: display page changed` log while PIR is high. [Encoder pulse screenshot](evidence/wokwi-encoder-pulses.png) records the original pulse check. The 2026-09-30 live run's ordered page sequence is recorded in [the live verification note](evidence/wokwi-live-run-2026-09-30.md).
3. [PIR high/low trace](evidence/wokwi-pir-cycle.vcd) records the sensor output pulse returning low. The live Wokwi serial observation included `Motion: detected` and `State: ACTIVE`; the active encoder screenshot also shows a PIR-high heartbeat during page-change handling.
4. [Buzzer high-temperature waveform](evidence/wokwi-buzzer-alarm-on.vcd) and [test screenshot](evidence/wokwi-buzzer-pwm.png) document the 32 C PWM output; [recovery waveform](evidence/wokwi-buzzer-recovery.vcd) and [screenshot](evidence/wokwi-buzzer-recovery.png) document return to low after the 23.5 C sample. [Pre-fix no-PWM screenshot](evidence/wokwi-buzzer-no-pwm.png) is retained as a diagnostic baseline.
5. Temporary fault observations: [no-blocking run](evidence/experiment-no-blocking.png), [raised-priority run](evidence/experiment-high-priority.png), and [mutex-bypass run](evidence/experiment-no-mutex.png). Production blocking and UART synchronization were restored afterward.
