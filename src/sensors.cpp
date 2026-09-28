#include "sensors.h"
#include "board.h"
#include "rtos_objects.h"
#include "logic.h"
#include "task.h"
#include <cstdio>

static bool waitForPin(GPIO_PinState level, uint32_t timeoutUs) {
  const uint32_t start = boardMicros();
  while (HAL_GPIO_ReadPin(GPIOB, DHT_PIN) != level) {
    if (static_cast<uint16_t>(boardMicros() - start) > timeoutUs) return false;
  }
  return true;
}

static bool readDht(float &temperature, float &humidity) {
  uint8_t bytes[5] = {};
  HAL_GPIO_WritePin(GPIOB, DHT_PIN, GPIO_PIN_RESET);
  vTaskDelay(pdMS_TO_TICKS(2));
  taskENTER_CRITICAL();
  HAL_GPIO_WritePin(GPIOB, DHT_PIN, GPIO_PIN_SET);
  bool ok = waitForPin(GPIO_PIN_RESET, 120) &&
            waitForPin(GPIO_PIN_SET, 120) &&
            waitForPin(GPIO_PIN_RESET, 120);
  for (int bit = 0; bit < 40 && ok; ++bit) {
    ok = waitForPin(GPIO_PIN_SET, 90);
    if (!ok) break;
    const uint32_t highStart = boardMicros();
    ok = waitForPin(GPIO_PIN_RESET, 100);
    if (!ok) break;
    bytes[bit / 8] = static_cast<uint8_t>((bytes[bit / 8] << 1) |
        (static_cast<uint16_t>(boardMicros() - highStart) > 50U ? 1U : 0U));
  }
  taskEXIT_CRITICAL();
  if (!ok || static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4])
    return false;
  const uint16_t rawHumidity = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
  const uint16_t rawTemperature = static_cast<uint16_t>(((bytes[2] & 0x7fU) << 8) | bytes[3]);
  humidity = rawHumidity / 10.0f;
  temperature = rawTemperature / 10.0f;
  if (bytes[2] & 0x80U) temperature = -temperature;
  return true;
}

void SensorTask(void *) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    SensorData data = {};
    data.dhtValid = readDht(data.temperature, data.humidity);
    data.lightLevel = lightPercentFromAdc(boardReadLightAdc());
    xQueueOverwrite(displaySensorQueue, &data);
    xQueueOverwrite(alarmSensorQueue, &data);
    char line[96];
    if (data.dhtValid) {
      const int temp10 = static_cast<int>(data.temperature * 10.0f);
      const int hum10 = static_cast<int>(data.humidity * 10.0f);
      std::snprintf(line, sizeof(line), "Sensor: %d.%d C, %d.%d %%, light %d %%\r\n",
                    temp10 / 10, temp10 % 10, hum10 / 10, hum10 % 10, data.lightLevel);
    } else {
      std::snprintf(line, sizeof(line), "Sensor: DHT checksum/timeout, light %d %%\r\n",
                    data.lightLevel);
    }
    boardLog(line);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
  }
}

