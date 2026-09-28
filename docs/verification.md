# Wokwi functional verification record

Build with `pio run -e bluepill_f103c8`, start Wokwi for VS Code, and record the **observed** OLED/serial/buzzer behavior before assigning PASS or FAIL. These rows are intentionally pending because a firmware build and host unit tests do not execute the Wokwi peripherals.

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

