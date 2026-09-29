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
    failure = "data-bit-high";
    ok = waitForPin(GPIO_PIN_RESET, 100);
    if (!ok) break;
    bytes[bit / 8] = static_cast<uint8_t>((bytes[bit / 8] << 1) |
        (static_cast<uint16_t>(boardMicros() - highStart) > 50U ? 1U : 0U));
  }
  taskEXIT_CRITICAL();
  if (!ok) return false;
  failure = "checksum";
  if (static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4])
    return false;
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
    data.lightLevel = lightPercentFromAdc(boardReadLightAdc());
    xQueueOverwrite(displaySensorQueue, &data);
    xQueueOverwrite(alarmSensorQueue, &data);
    if (data.dhtValid) {
      const int temp10 = static_cast<int>(data.temperature * 10.0f);
      const int hum10 = static_cast<int>(data.humidity * 10.0f);
      std::snprintf(line, sizeof(line), "Sensor: %d.%d C, %d.%d %%, light %d %%\r\n",
                    temp10 / 10, temp10 % 10, hum10 / 10, hum10 % 10, data.lightLevel);
    } else {
      std::snprintf(line, sizeof(line), "Sensor: DHT failure at %s, light %d %%\r\n",
                    dhtFailure, data.lightLevel);
    }
    boardLog(line);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
  }
}
