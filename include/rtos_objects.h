#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "logic.h"

struct SensorData {
  float temperature;
  float humidity;
  int lightLevel;
  bool dhtValid;
};

constexpr EventBits_t EVENT_ACTIVE = (1U << 0);
constexpr EventBits_t EVENT_MOTION = (1U << 1);
constexpr EventBits_t EVENT_ALARM = (1U << 2);

extern QueueHandle_t displaySensorQueue;
extern QueueHandle_t alarmSensorQueue;
extern QueueHandle_t displayModeQueue;
extern QueueHandle_t motionDisplayQueue;
extern QueueHandle_t encoderQueue;
extern SemaphoreHandle_t serialMutex;
extern EventGroupHandle_t systemEvents;

bool createRtosObjects();

