#pragma once

#include <stdint.h>

enum class DisplayMode : uint8_t { TEMPERATURE, HUMIDITY, LIGHT, MOTION };
enum class AlarmState : uint8_t { NORMAL, LOW_TEMPERATURE, HIGH_TEMPERATURE };
enum class SystemState : uint8_t { ACTIVE, INACTIVE };

AlarmState evaluateTemperature(float temperature);
DisplayMode nextDisplayMode(DisplayMode mode);
DisplayMode previousDisplayMode(DisplayMode mode);
SystemState evaluateSystemState(SystemState current, bool motion,
                                uint32_t elapsedSinceMotionMs,
                                uint32_t timeoutMs = 15000);
int lightPercentFromAdc(uint16_t raw);

