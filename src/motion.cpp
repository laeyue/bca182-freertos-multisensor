#include "motion.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"

void MotionTask(void *) {
  TickType_t lastWake = xTaskGetTickCount();
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
  }
}

