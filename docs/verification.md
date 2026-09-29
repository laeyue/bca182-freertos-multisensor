# Wokwi functional verification record

Build with `pio run -e bluepill_f103c8`, start Wokwi for VS Code, and record the **observed** OLED/serial/buzzer behavior before assigning PASS or FAIL. These rows are intentionally pending because a firmware build and host unit tests do not execute the Wokwi peripherals.

## Startup wakeup diagnosis

In a live Wokwi run, the serial output reached `Scheduler running` and
`Sensor: sampling DHT22`, then stopped. After 12.2 simulated seconds it had
not printed `Sensor: DHT start delay finished` or the MotionTask heartbeat.
The missing sensor line precedes DHT pin decoding, so a DHT22 response or
checksum failure cannot explain this stall. The MotionTask heartbeat also
depends on periodic task wakeups. This points to the FreeRTOS tick/wakeup path,
although serial logging remains a possible confounder until the wakeup path is
verified independently.

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

## DHT22 bus fix

The failed read was waiting for the sensor's response-high edge. PB12 stayed in
output open-drain mode after the host released the bus, so the STM32F1 internal
pull-up was not enabled. The [AM2302 datasheet](https://files.seeedstudio.com/wiki/Grove-Temperature_and_Humidity_Sensor_Pro/res/AM2302-EN.pdf)
recommends an approximately 5.1 kΩ external pull-up and releasing the bus by
switching the MCU pin to input. The diagram now includes that resistor, and the
driver switches PB12 to input-pull-up for receive and waits two seconds after
power-up before the first sample.

Verified in a freshly reloaded VS Code Wokwi run: the default 24 °C / 40% sensor
settings repeatedly produced checksum-valid `24.0 C, 40.0 %` readings. While
the simulation was running, changing the sensor controls to 31 °C / 53% produced
`31.0 C, 53.0 %` on subsequent samples. MotionTask heartbeats continued during
the reads.

| ID | Stimulus | Expected result | Actual observation | Result |
| --- | --- | --- | --- | --- |
| FT-01 | Set DHT22 to 28 C, select Temperature | OLED shows about 28.0 C within 2 s | Not observed | Pending |
| FT-02 | Set DHT22 to 65%, select Humidity | OLED shows about 65.0% within 2 s | Not observed | Pending |
| FT-03 | Change LDR lux control, select Light | Relative brightness percentage changes | Not observed | Pending |
| FT-04 | Rotate encoder clockwise | Temperature -> Humidity -> Light -> Motion -> Temperature | Not observed | Pending |
| FT-05 | Rotate encoder counterclockwise | Reverse sequence with wraparound | Not observed | Pending |
| FT-06 | Set DHT22 above 30 C | Buzzer sounds, OLED alarm indicator appears | Not observed | Pending |
| FT-07 | Return DHT22 to 24 C | Buzzer stops within next sensor sample | Not observed | Pending |
| FT-08 | Trigger PIR | State log shows ACTIVE and OLED on | Not observed | Pending |
| FT-09 | Wait 15 s after PIR output returns low | State log shows INACTIVE and OLED blanks | Not observed | Pending |
| FT-10 | Trigger PIR while INACTIVE | OLED restores and state log shows ACTIVE | Not observed | Pending |

## Deliberate FreeRTOS fault experiments

These experiments should be performed in a temporary branch or with reversible edits. The shipping code keeps all blocking and synchronization intact. Record real observations; predicted effects alone do not fulfill the experiment requirement.

| Experiment | Temporary change | What to observe | Actual observation | Status |
| --- | --- | --- | --- | --- |
| Remove blocking | Remove `vTaskDelayUntil()` in `MotionTask` | CPU load, lower-priority display/sensor latency, starvation | Not performed | Pending |
| Raise priority | Give frequent MotionTask an unnecessarily high priority while reducing its delay | Scheduling responsiveness of other tasks | Not performed | Pending |
| Remove mutex | Bypass `serialMutex` in `boardLog` while several tasks log | Interleaved UART lines | Not performed | Pending |

## Evidence to capture

1. Wokwi full-circuit screenshot and OLED screenshot for each selected page.
2. Serial log showing sensor updates and ACTIVE/INACTIVE transition.
3. Short note about whether buzzer sound is audible at alarm boundaries.
4. Before/after scheduling observations for each deliberate fault experiment.
