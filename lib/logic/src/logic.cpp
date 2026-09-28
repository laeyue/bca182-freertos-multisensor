#include "logic.h"

AlarmState evaluateTemperature(float temperature) {
  if (temperature < 18.0f) return AlarmState::LOW_TEMPERATURE;
  if (temperature > 30.0f) return AlarmState::HIGH_TEMPERATURE;
  return AlarmState::NORMAL;
}

DisplayMode nextDisplayMode(DisplayMode mode) {
  return static_cast<DisplayMode>((static_cast<uint8_t>(mode) + 1U) % 4U);
}

DisplayMode previousDisplayMode(DisplayMode mode) {
  return static_cast<DisplayMode>((static_cast<uint8_t>(mode) + 3U) % 4U);
}

SystemState evaluateSystemState(SystemState current, bool motion,
                                uint32_t elapsedSinceMotionMs,
                                uint32_t timeoutMs) {
  if (motion) return SystemState::ACTIVE;
  if (current == SystemState::ACTIVE && elapsedSinceMotionMs >= timeoutMs)
    return SystemState::INACTIVE;
  return current;
}

int lightPercentFromAdc(uint16_t raw) {
  if (raw > 4095U) raw = 4095U;
  // Wokwi's LDR module AO falls as illumination increases. This is a
  // relative brightness indicator, not calibrated lux.
  return static_cast<int>(((4095U - raw) * 100U + 2047U) / 4095U);
}

