#include "board.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "motion.h"
#include "alarm.h"
#include "system_state.h"
#include "task.h"

extern "C" void xPortSysTickHandler(void);

extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    xPortSysTickHandler();
}

extern "C" void vApplicationMallocFailedHook(void) { taskDISABLE_INTERRUPTS(); for (;;) {} }
extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *) {
  taskDISABLE_INTERRUPTS();
  for (;;) {}
}

void app_main() {
  if (!createRtosObjects()) for (;;) {}
  boardLog("BCA182 FreeRTOS Multisensor System starting...\r\n");
  const bool created =
      xTaskCreate(MotionTask, "MotionTask", 160, nullptr, 3, nullptr) == pdPASS &&
      xTaskCreate(InputTask, "InputTask", 160, nullptr, 3, nullptr) == pdPASS &&
      xTaskCreate(StateTask, "StateTask", 192, nullptr, 3, nullptr) == pdPASS &&
      xTaskCreate(SensorTask, "SensorTask", 256, nullptr, 2, nullptr) == pdPASS &&
      xTaskCreate(AlarmTask, "AlarmTask", 192, nullptr, 2, nullptr) == pdPASS &&
      xTaskCreate(DisplayTask, "DisplayTask", 320, nullptr, 1, nullptr) == pdPASS;
  if (!created) for (;;) {}
  vTaskStartScheduler();
  for (;;) {}
}

int main() {
  boardInit();
  app_main();
}

