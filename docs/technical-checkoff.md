# Technical checkoff notes

Use these notes to explain the design in your own words. The source and actual simulator behavior are the final authority.

1. **Why SensorTask?** DHT22 and ADC acquisition have a 2-second periodic deadline independent of display transfers or encoder events.
2. **Why these priorities?** Motion, encoder and state handling need short response latency (priority 3); sampling and alarm can tolerate the 2-second update cadence (2); the OLED is slower and less urgent (1).
3. **Why `vTaskDelayUntil()`?** It schedules from the previous planned wake time, avoiding cumulative execution-time drift.
4. **What happens during a delay?** The task is Blocked and uses no CPU. At the wake tick it becomes Ready; the scheduler selects it when eligible.
5. **What crosses the sensor queues?** A copied `SensorData`: temperature, humidity, relative light percent and DHT validity.
6. **Why two queues?** DisplayTask and AlarmTask each need the newest sample. Consuming from one shared queue would make one task steal the other's update.
7. **What does the mutex protect?** The shared USART1 transmit operation used by five tasks' diagnostic log calls.
8. **What race can occur?** A task switch during UART output can mix bytes from two messages unless each write holds the mutex.
9. **What are event bits?** MotionTask pulses EVENT_MOTION; StateTask owns EVENT_ACTIVE; AlarmTask owns EVENT_ALARM. Display, input and alarm tasks read relevant level bits.
10. **Who owns the OLED?** DisplayTask alone, so I2C frame writes cannot interleave.
11. **What if a high-priority task never blocks?** It can keep lower-priority sensor, alarm and display tasks Ready indefinitely, causing starvation.
12. **Ready versus Blocked?** Ready can run once selected; Blocked waits for a time, message or event.
13. **What do unit tests verify?** Alarm thresholds including exact boundaries, encoder wraparound, activity-state transitions, and brightness endpoint conversion. They do not prove HAL or Wokwi behavior.
14. **What did static analysis find?** Two project style issues were fixed. Five remaining low messages come from C-style casts inside STM32 HAL/FreeRTOS macros. No medium or high issues were reported.
15. **What changes on hardware?** Voltage levels, pull-ups, buzzer drive, power supply, sensor calibration, timing margins and EMI need electrical validation. Wokwi models do not establish those properties.

