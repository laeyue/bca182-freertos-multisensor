#include "input.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"

extern "C" void EXTI1_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ENCODER_CLK_PIN); }

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (pin != ENCODER_CLK_PIN || !encoderQueue ||
      xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return;
  const int8_t direction = HAL_GPIO_ReadPin(GPIOA, ENCODER_DT_PIN) == GPIO_PIN_SET ? 1 : -1;
  BaseType_t wake = pdFALSE;
  xQueueSendFromISR(encoderQueue, &direction, &wake);
  portYIELD_FROM_ISR(wake);
}

void InputTask(void *) {
  DisplayMode mode = DisplayMode::TEMPERATURE;
  TickType_t lastEdge = 0;
  for (;;) {
    int8_t direction = 0;
    if (xQueueReceive(encoderQueue, &direction, portMAX_DELAY) != pdTRUE) continue;
    const TickType_t now = xTaskGetTickCount();
    if (now - lastEdge < pdMS_TO_TICKS(3)) continue;
    lastEdge = now;
    if ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) == 0) continue;
    mode = direction > 0 ? nextDisplayMode(mode) : previousDisplayMode(mode);
    xQueueOverwrite(displayModeQueue, &mode);
    boardLog("Input: display page changed\r\n");
  }
}

