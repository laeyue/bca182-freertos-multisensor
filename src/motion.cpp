#include "motion.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"

void MotionTask(void *) {
  boardLog("Scheduler running\r\n");
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t heartbeats = 0;
  bool previous = false;
  for (;;) {
    const bool detected = HAL_GPIO_ReadPin(GPIOB, PIR_PIN) == GPIO_PIN_SET;
    if (detected) xEventGroupSetBits(systemEvents, EVENT_MOTION);
    if (detected != previous) {
      xQueueOverwrite(motionDisplayQueue, &detected);
      boardLog(detected ? "Motion: detected\r\n" : "Motion: cleared\r\n");
      previous = detected;
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(50));
    if (++heartbeats % 20U == 0U) boardLog("Motion: heartbeat\r\n");
  }
}
