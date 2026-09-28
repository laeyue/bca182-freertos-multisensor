# Hackster.io draft: FreeRTOS STM32 Room Multisensor

**Publication state:** Draft. Publish after the Wokwi functional record and fault experiments have actual observations, and after a public GitHub repository exists. Replace the bracketed link and author information with the student's own details.

## Project Overview

This project uses an STM32F103C8 Blue Pill with native FreeRTOS tasks to monitor simulated room temperature, humidity, relative brightness and motion. A rotary encoder navigates an SSD1306 OLED. A 500 Hz buzzer signals temperatures outside 18-30 C, and a 15-second inactivity timer blanks the display until PIR activity returns.

## Motivation

A room monitor is a compact way to study task scheduling and communication. Each peripheral has a distinct timing and response need: a DHT22 sample arrives slowly, encoder input should feel immediate, and the display can update at lower priority.

## Components and Circuit

The Wokwi circuit contains a Blue Pill, DHT22, LDR module, PIR, KY-040 encoder, SSD1306 OLED and buzzer. The full wiring is in `diagram.json` and the pin table in the README. [Insert a screenshot from the verified simulator here.]

## FreeRTOS Architecture

Six tasks separate sensor sampling, encoder handling, PIR monitoring, state transitions, alarm control and OLED rendering. The sensor sends independent newest-value messages to display and alarm queues. The state task owns the ACTIVE bit in an event group; the alarm task owns the ALARM bit. A mutex serializes diagnostic writes over USART1. A 2-second `vTaskDelayUntil()` schedule keeps sensor sampling anchored to fixed wake times.

## How It Works

The DHT22 is decoded through timed GPIO pulses and checksum verification. ADC1 reads the photoresistor module; its output is shown as a relative 0-100% brightness indicator, not calibrated lux. An EXTI interrupt records encoder steps for InputTask. The OLED has a single owner, DisplayTask. TIM4 channel 3 supplies a square wave for the piezo buzzer.

## Testing and Verification

The STM32Cube target builds in PlatformIO. Fifteen Unity tests pass for alarm boundaries, navigation, state changes and brightness conversion. Cppcheck reports no high or medium findings; five low style messages are associated with vendor macro expansions. [Insert verified Wokwi FT-01 to FT-10 observations and fault-experiment results here after running them.]

## Challenges and Lessons Learned

One queue with two destructive consumers would split sensor updates, so separate one-slot mailboxes were used. The DHT22 requires microsecond timing, and the current bit-banged critical section trades interrupt latency for implementation simplicity. Using a timer PWM output matters because a steady high level does not create a piezo tone.

## Limitations and Future Improvements

Wokwi behavior needs an interactive verification run before performance claims can be made. A timer input capture for DHT pulses, sensor fault alert, calibrated light conversion and measured task latency would strengthen a hardware version.

## Source Code

[Insert public GitHub repository URL here after publication.]

## References

PlatformIO STM32Cube documentation; Wokwi Blue Pill and component documentation; FreeRTOS kernel included with STM32CubeF1; BCA182 Laboratory Activity 1 (MSU-IIT, September 2026).

