# Hackster.io project draft

**Publication state:** Draft prepared; not published.

## Listing details

- **Project name:** Real-Time FreeRTOS Room Multisensor
- **Short description:** An STM32 Blue Pill room monitor built with six FreeRTOS tasks, a DHT22, LDR, PIR sensor, rotary encoder, SSD1306 OLED, and PWM buzzer.
- **Repository:** https://github.com/laeyue/bca182-freertos-multisensor
- **Course attribution:** BCA182, Mindanao State University - Iligan Institute of Technology (MSU-IIT)
- **Collaborator field:** Add Prof. Paul Rodolf P. Castor using the matching Hackster profile, as required by the laboratory handout.
- **Tags:** FreeRTOS, STM32, embedded, sensors, Wokwi, PlatformIO
- **License:** Select the license for the Hackster project and add a matching repository `LICENSE` before publication. The repository does not currently declare one.

## Project Overview

This project monitors simulated room temperature, humidity, relative brightness, and motion with an STM32F103C8 Blue Pill. A KY-040 rotary encoder selects one of four pages on an SSD1306 OLED. A 500 Hz buzzer output indicates temperatures outside the 18-30 C range. After 15 seconds without motion, the display blanks; PIR activity restores the active display.

The project was built for BCA182 Laboratory Activity 1 to demonstrate FreeRTOS task scheduling, inter-task communication, periodic work, and state management on an STM32 target.

## Components and Tools

- STM32F103C8 Blue Pill
- DHT22 temperature and humidity sensor
- Photoresistor module (LDR)
- PIR motion sensor
- KY-040 rotary encoder
- SSD1306 I2C OLED
- Piezo buzzer
- PlatformIO, STM32Cube HAL, FreeRTOS, Unity, and Wokwi

The full circuit and pin assignment are in the public repository. The circuit uses PB12 for DHT22 data with a 5.1 kOhm pull-up, PA0 for LDR analog output, PB13 for PIR output, PA1/PA2 for encoder CLK/DT, PB6/PB7 for I2C1 OLED, PB8 for TIM4 buzzer PWM, and PA9 for USART1 logging.

![Wokwi circuit](https://raw.githubusercontent.com/laeyue/bca182-freertos-multisensor/main/docs/evidence/wokwi-full-circuit.png)

## FreeRTOS Architecture

Six tasks divide the work: MotionTask reads PIR state, InputTask handles encoder steps, StateTask owns ACTIVE/INACTIVE state, SensorTask samples DHT22 and LDR, AlarmTask controls the buzzer, and DisplayTask owns the OLED. SensorTask writes the newest sensor data to separate one-item queues for DisplayTask and AlarmTask. The state and alarm event bits live in an event group. A mutex serializes USART1 messages.

The sensor task uses `vTaskDelayUntil()` with a two-second period. The 15-second inactivity timeout is handled by StateTask. Each task blocks on a queue, event, or timed delay between work.

## How It Works

The DHT22 driver releases PB12 into input-pull-up mode and samples each data bit 40 microseconds after its rising edge. This timing separates the sensor's short zero pulse from its longer one pulse. The driver validates the five-byte checksum before publishing a reading. The LDR module is reported as relative brightness, not calibrated lux. Encoder direction is decoded from CLK and DT transitions. DisplayTask renders one selected page. TIM4 channel 3 generates the buzzer waveform at about 500 Hz.

![Temperature page](https://raw.githubusercontent.com/laeyue/bca182-freertos-multisensor/main/docs/evidence/wokwi-oled-28c-temperature.png)

![Humidity page](https://raw.githubusercontent.com/laeyue/bca182-freertos-multisensor/main/docs/evidence/wokwi-oled-humidity.png)

![Light page](https://raw.githubusercontent.com/laeyue/bca182-freertos-multisensor/main/docs/evidence/wokwi-oled-light.png)

![Motion page](https://raw.githubusercontent.com/laeyue/bca182-freertos-multisensor/main/docs/evidence/wokwi-oled-motion.png)

## Testing and Verification

The firmware builds for `bluepill_f103c8`. All 15 native Unity tests pass for alarm thresholds, page navigation logic, activity-state transitions, and brightness conversion. PlatformIO static analysis reported zero high and zero medium findings, with 16 low C-style-cast findings at STM32 HAL or FreeRTOS macro call sites.

Live VS Code Wokwi runs showed checksum-valid DHT22 readings, changing LDR output, all four OLED pages, PIR ACTIVE/INACTIVE transitions, and page-change logs for both encoder directions. On 2026-09-30, the full clockwise sequence (Temperature -> Humidity -> Light -> Motion -> Temperature) and reverse sequence (Temperature -> Motion -> Light -> Humidity -> Temperature) were observed while ACTIVE. At 33.1 C / 66.0%, the OLED showed ALARM and the Wokwi buzzer activity icon appeared; the retained PB8 trace measures about 500 Hz at 50% duty cycle, with a recovery trace returning low after a normal-temperature sample. These are simulator results; no independent physical sound-level measurement or physical-board validation has been made. The test-only PIR hold was restored to its normal five-second value after the run.

The Wokwi functional checks for encoder wraparound and the high-temperature alarm passed. The independent physical sound level remains unmeasured, and the design has not been validated on physical hardware.

Reversible fault experiments showed that removing MotionTask's blocking delay or raising its priority suppressed sensor output during the observed runs. Bypassing the UART mutex did not reproduce visible line interleaving in the short capture. These are observations from the simulator; they do not replace testing with physical hardware.

The full [functional verification record](https://github.com/laeyue/bca182-freertos-multisensor/blob/main/docs/verification.md) links the screenshots and VCD traces and lists these limits. The public [source repository](https://github.com/laeyue/bca182-freertos-multisensor) includes the circuit, firmware, report, and evidence.

## Challenges and Lessons Learned

A single sensor queue with two consumers would split updates between the display and alarm tasks, so the firmware uses separate one-item mailboxes. The DHT22 needs microsecond timing, and the bit-banged critical section trades interrupt latency for a simpler driver. A steady GPIO level cannot drive a piezo tone; the buzzer needs timer PWM.

## Limitations and Future Improvements

The design has not been validated on a physical board, and no independent acoustic level was measured. Future work could add timer input capture for DHT pulses, calibrated light measurements, measured task latency, and a sensor fault alarm.

## References

BCA182 Laboratory Activity 1 (MSU-IIT, 2026); Aosong AM2302 Technical Manual; STM32CubeF1 documentation and FreeRTOS kernel; PlatformIO STM32Cube documentation; Wokwi component documentation.
