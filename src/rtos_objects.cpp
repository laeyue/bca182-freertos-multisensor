#include "rtos_objects.h"

QueueHandle_t displaySensorQueue = nullptr;
QueueHandle_t alarmSensorQueue = nullptr;
QueueHandle_t displayModeQueue = nullptr;
QueueHandle_t motionDisplayQueue = nullptr;
QueueHandle_t encoderQueue = nullptr;
SemaphoreHandle_t serialMutex = nullptr;
EventGroupHandle_t systemEvents = nullptr;

bool createRtosObjects() {
  displaySensorQueue = xQueueCreate(1, sizeof(SensorData));
  alarmSensorQueue = xQueueCreate(1, sizeof(SensorData));
  displayModeQueue = xQueueCreate(1, sizeof(DisplayMode));
  motionDisplayQueue = xQueueCreate(1, sizeof(bool));
  encoderQueue = xQueueCreate(8, sizeof(int8_t));
  serialMutex = xSemaphoreCreateMutex();
  systemEvents = xEventGroupCreate();
  if (!displaySensorQueue || !alarmSensorQueue || !displayModeQueue ||
      !motionDisplayQueue || !encoderQueue || !serialMutex || !systemEvents)
    return false;
  xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
  return true;
}

