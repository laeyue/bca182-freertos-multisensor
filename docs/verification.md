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

| ID | Stimulus | Expected result | Actual observation | Result |
| --- | --- | --- | --- | --- |
| FT-01 | Set DHT22 to 28 C, select Temperature | OLED shows about 28.0 C within 2 s | Temperature page showed 32.0 C and ACTIVE on the OLED; serial reads at 24 C and 32 C were valid, but the requested 28 C OLED value was not tested | Pending |
| FT-02 | Set DHT22 to 65%, select Humidity | OLED shows about 65.0% within 2 s | Serial output verified 65.0% at 32 C; Humidity OLED page was not observed | Pending |
| FT-03 | Change LDR lux control, select Light | Relative brightness percentage changes | LDR output changed from 76% (ADC 980 at 501 lux) to 97% (ADC 130 at 13,183 lux); Light OLED page was not observed | Pending |
| FT-04 | Rotate encoder clockwise | Temperature -> Humidity -> Light -> Motion -> Temperature | Not observed | Pending |
| FT-05 | Rotate encoder counterclockwise | Reverse sequence with wraparound | Not observed | Pending |
| FT-06 | Set DHT22 above 30 C | Buzzer sounds, OLED alarm indicator appears | DHT read returned 32.0 C; alarm output and audible buzzer were not confirmed | Pending |
| FT-07 | Return DHT22 to 24 C | Buzzer stops within next sensor sample | Not observed | Pending |
| FT-08 | Trigger PIR | State log shows ACTIVE and OLED on | Wokwi Simulate motion restored the OLED Temperature page with ACTIVE visible; matching ACTIVE serial log was not captured | Pending |
| FT-09 | Wait 15 s after PIR output returns low | State log shows INACTIVE and OLED blanks | `State: INACTIVE` was observed after inactivity, with the OLED blank | PASS |
| FT-10 | Trigger PIR while INACTIVE | OLED restores and state log shows ACTIVE | OLED appeared to restore after the Wokwi PIR control; matching ACTIVE log was not captured | Pending |

## Deliberate FreeRTOS fault experiments

These experiments should be performed in a temporary branch or with reversible edits. The shipping code keeps all blocking and synchronization intact. Record real observations; predicted effects alone do not fulfill the experiment requirement.

| Experiment | Temporary change | What to observe | Actual observation | Status |
| --- | --- | --- | --- | --- |
| Remove blocking | Remove `vTaskDelayUntil()` in `MotionTask` | CPU load, lower-priority display/sensor latency, starvation | Not performed | Pending |
| Raise priority | Give frequent MotionTask an unnecessarily high priority while reducing its delay | Scheduling responsiveness of other tasks | Not performed | Pending |
| Remove mutex | Bypass `serialMutex` in `boardLog` while several tasks log | Interleaved UART lines | Not performed | Pending |

## Evidence to capture

1. [Wokwi full-circuit screenshot](evidence/wokwi-full-circuit.png) and [active OLED Temperature screenshot](evidence/wokwi-oled-active.png); Humidity, Light and Motion pages still need screenshots.
2. Serial log showing sensor updates and ACTIVE/INACTIVE transition.
3. Short note about whether buzzer sound is audible at alarm boundaries.
4. Before/after scheduling observations for each deliberate fault experiment.
