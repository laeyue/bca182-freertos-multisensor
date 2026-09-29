#include "sensors.h"
#include "board.h"
#include "rtos_objects.h"
#include "logic.h"
#include "task.h"
#include <cstdio>

static void driveDhtLow() {
  GPIO_InitTypeDef gpio = {};
  gpio.Pin = DHT_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_OD;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &gpio);
  HAL_GPIO_WritePin(GPIOB, DHT_PIN, GPIO_PIN_RESET);
}

static void releaseDhtBus() {
  GPIO_InitTypeDef gpio = {};
  gpio.Pin = DHT_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &gpio);
}

static bool waitForPin(GPIO_PinState level, uint32_t timeoutUs) {
  const uint32_t start = boardMicros();
  uint32_t spins = 0;
  const uint32_t spinLimit = timeoutUs * 64U;
  while (HAL_GPIO_ReadPin(GPIOB, DHT_PIN) != level) {
    if (static_cast<uint16_t>(boardMicros() - start) > timeoutUs ||
        ++spins >= spinLimit) return false;
  }
  return true;
}

static bool readDht(float &temperature, float &humidity, const char *&failure) {
  uint8_t bytes[5] = {};
  uint8_t decodedOnes = 0;
  uint16_t shortestPulse = UINT16_MAX;
  uint16_t longestPulse = 0;
  driveDhtLow();
  vTaskDelay(pdMS_TO_TICKS(2));
  boardLog("Sensor: DHT start delay finished\r\n");
  taskENTER_CRITICAL();
  releaseDhtBus();
  failure = "response-low";
  bool ok = waitForPin(GPIO_PIN_RESET, 120);
  if (ok) {
    failure = "response-high";
    ok = waitForPin(GPIO_PIN_SET, 120);
  }
  if (ok) {
    failure = "first-data-low";
    ok = waitForPin(GPIO_PIN_RESET, 120);
  }
  for (int bit = 0; bit < 40 && ok; ++bit) {
    failure = "data-bit-low";
    ok = waitForPin(GPIO_PIN_SET, 90);
    if (!ok) break;
    const uint32_t highStart = boardMicros();
    // Sample between the AM2302's 26-28 us zero pulse and ~70 us one pulse.
    // Sampling at a fixed midpoint avoids classifying bits from two polled
    // edge timestamps, whose loop jitter can skew a measured pulse width.
    uint32_t sampleSpins = 0;
    while (static_cast<uint32_t>(boardMicros() - highStart) < 40U) {
      if (++sampleSpins >= 40U * 64U) {
        failure = "data-bit-sample";
        ok = false;
        break;
      }
    }
    if (!ok) break;
    const bool decodedOne = HAL_GPIO_ReadPin(GPIOB, DHT_PIN) == GPIO_PIN_SET;
    failure = "data-bit-high";
    ok = waitForPin(GPIO_PIN_RESET, 100);
    if (!ok) break;
    const uint16_t pulseWidth = static_cast<uint16_t>(boardMicros() - highStart);
    if (pulseWidth < shortestPulse) shortestPulse = pulseWidth;
    if (pulseWidth > longestPulse) longestPulse = pulseWidth;
    if (decodedOne) ++decodedOnes;
    bytes[bit / 8] = static_cast<uint8_t>((bytes[bit / 8] << 1) | (decodedOne ? 1U : 0U));
  }
  taskEXIT_CRITICAL();
  if (!ok) return false;
  failure = "checksum";
  const uint8_t checksum = static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
  if (checksum != bytes[4]) {
    char line[128];
    std::snprintf(line, sizeof(line),
                  "Sensor: DHT checksum mismatch sum %02X got %02X, data %02X %02X %02X %02X, bits %u, pulse %u-%u us\r\n",
                  static_cast<unsigned int>(checksum), static_cast<unsigned int>(bytes[4]),
                  static_cast<unsigned int>(bytes[0]), static_cast<unsigned int>(bytes[1]),
                  static_cast<unsigned int>(bytes[2]), static_cast<unsigned int>(bytes[3]),
                  static_cast<unsigned int>(decodedOnes), static_cast<unsigned int>(shortestPulse),
                  static_cast<unsigned int>(longestPulse));
    boardLog(line);
    return false;
  }
  const uint16_t rawHumidity = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
  const uint16_t rawTemperature = static_cast<uint16_t>(((bytes[2] & 0x7fU) << 8) | bytes[3]);
  humidity = rawHumidity / 10.0f;
  temperature = rawTemperature / 10.0f;
  if (bytes[2] & 0x80U) temperature = -temperature;
  failure = "";
  return true;
}

void SensorTask(void *) {
  boardLog("Sensor: waiting for DHT22 power-up\r\n");
  vTaskDelay(pdMS_TO_TICKS(2000));
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    SensorData data = {};
    const char *dhtFailure = "unknown";
    char line[96];
    boardLog("Sensor: sampling DHT22\r\n");
    data.dhtValid = readDht(data.temperature, data.humidity, dhtFailure);
    if (data.dhtValid) boardLog("Sensor: DHT22 read complete\r\n");
    const uint16_t lightAdc = boardReadLightAdc();
    data.lightLevel = lightPercentFromAdc(lightAdc);
    xQueueOverwrite(displaySensorQueue, &data);
    xQueueOverwrite(alarmSensorQueue, &data);
    if (data.dhtValid) {
      const int temp10 = static_cast<int>(data.temperature * 10.0f);
      const int hum10 = static_cast<int>(data.humidity * 10.0f);
      std::snprintf(line, sizeof(line), "Sensor: %d.%d C, %d.%d %%\r\n",
                    temp10 / 10, temp10 % 10, hum10 / 10, hum10 % 10);
    } else {
      std::snprintf(line, sizeof(line), "Sensor: DHT failure at %s\r\n", dhtFailure);
    }
    boardLog(line);
    std::snprintf(line, sizeof(line), "Sensor: light %d %% ADC %u\r\n", data.lightLevel,
                  static_cast<unsigned int>(lightAdc));
    boardLog(line);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
  }
}
