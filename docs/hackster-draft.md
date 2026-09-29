# Hackster.io draft: FreeRTOS STM32 Room Multisensor

**Publication state:** Local draft only. The full Wokwi functional matrix and deliberate fault experiments are not complete. Replace the repository link and author details with the student's own information before any publication.

## Project Overview

This project uses an STM32F103C8 Blue Pill with native FreeRTOS tasks to monitor simulated room temperature, humidity, relative brightness and motion. A rotary encoder navigates an SSD1306 OLED. A 500 Hz buzzer signals temperatures outside 18-30 C, and a 15-second inactivity timer blanks the display until PIR activity returns.

## Motivation

A room monitor is a compact way to study task scheduling and communication. Each peripheral has a distinct timing and response need: a DHT22 sample arrives slowly, encoder input should feel immediate, and the display can update at lower priority.

## Components and Circuit

The Wokwi circuit contains a Blue Pill, DHT22, LDR module, PIR, KY-040 encoder, SSD1306 OLED and buzzer. The full wiring is in `diagram.json` and the pin table in the README. Local screenshots document the full circuit and Temperature, Humidity, Light and Motion OLED pages. They remain in the repository as verification evidence.

![Wokwi full circuit](evidence/wokwi-full-circuit.png)

![OLED Temperature page at 27.8 C](evidence/wokwi-oled-28c-temperature.png)

![OLED Humidity page at 65 percent](evidence/wokwi-oled-humidity.png)

![OLED Light page](evidence/wokwi-oled-light.png)

![OLED Motion page](evidence/wokwi-oled-motion.png)

## FreeRTOS Architecture

Six tasks separate sensor sampling, encoder handling, PIR monitoring, state transitions, alarm control and OLED rendering. The sensor sends independent newest-value messages to display and alarm queues. The state task owns the ACTIVE bit in an event group; the alarm task owns the ALARM bit. A mutex serializes diagnostic writes over USART1. A 2-second `vTaskDelayUntil()` schedule keeps sensor sampling anchored to fixed wake times.

## How It Works

The DHT22 decoder samples each high pulse 40 us after its rising edge, between the 26-28 us zero pulse and approximately 70 us one pulse, then checks the five-byte checksum. In VS Code Wokwi, repeated checksum-valid readings were observed at 24 C / 40% and 32 C / 65%. ADC1 reads the photoresistor module; its output is shown as relative brightness, not calibrated lux. An EXTI interrupt records encoder steps for InputTask. The OLED has a single owner, DisplayTask. TIM4 channel 3 supplies a square wave for the piezo buzzer.

## Testing and Verification

The STM32Cube target builds in PlatformIO. Fifteen Unity tests pass for alarm boundaries, navigation, state changes and brightness conversion. `pio check` reports no high or medium findings and 16 low style findings, all C-style cast reports from STM32 HAL or FreeRTOS macro expansion. Wokwi confirmed DHT22 serial readings, LDR response, all four OLED pages and the inactive timeout. Encoder page changes were observed, though exact direction and wraparound evidence is incomplete. PIR reactivation appeared on the OLED, but its ACTIVE serial transition was not captured. The temperature alarm and audible buzzer response remain unverified. The three deliberate fault experiments have not been run.

## Challenges and Lessons Learned

One queue with two destructive consumers would split sensor updates, so separate one-slot mailboxes were used. The DHT22 requires microsecond timing, and the current bit-banged critical section trades interrupt latency for implementation simplicity. Using a timer PWM output matters because a steady high level does not create a piezo tone.

## Limitations and Future Improvements

The local Wokwi run covers DHT22 decoding, LDR response, OLED page rendering and inactivity, but not every encoder, PIR-log or buzzer test. A timer input capture for DHT pulses, sensor fault alert, calibrated light conversion and measured task latency would strengthen a hardware version.

## Source Code

[Insert public GitHub repository URL here after publication.]

## References

PlatformIO STM32Cube documentation; Wokwi Blue Pill and component documentation; FreeRTOS kernel included with STM32CubeF1; BCA182 Laboratory Activity 1 (MSU-IIT, September 2026).
