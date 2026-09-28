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

namespace {
// SysTick starts before the first task runs. Keep the ISR independent of the
// FreeRTOS API while the kernel is still initializing.
volatile bool schedulerStarted = false;

bool createTask(TaskFunction_t function, const char *name,
                uint16_t stackWords, UBaseType_t priority) {
  if (xTaskCreate(function, name, stackWords, nullptr, priority, nullptr) == pdPASS)
    return true;
  boardLog("Startup failed: cannot create ");
  boardLog(name);
  boardLog("\r\n");
  return false;
}
}

extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
  if (schedulerStarted) xPortSysTickHandler();
}

extern "C" void vApplicationMallocFailedHook(void) { taskDISABLE_INTERRUPTS(); for (;;) {} }
extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *) {
  taskDISABLE_INTERRUPTS();
  for (;;) {}
}

extern "C" void HardFault_Handler(void) {
  static const char message[] = "HardFault\r\n";
  HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(const_cast<char *>(message)),
                    sizeof(message) - 1, 100);
  for (;;) {}
}

void app_main() {
  if (!createRtosObjects()) for (;;) {}
  boardLog(SCB->VTOR == FLASH_BASE ? "Vector table: flash\r\n"
                                   : "Vector table: zero\r\n");
  boardLog("BCA182 FreeRTOS Multisensor System starting...\r\n");
  const bool created =
      createTask(MotionTask, "MotionTask", 160, 3) &&
      createTask(InputTask, "InputTask", 160, 3) &&
      createTask(StateTask, "StateTask", 192, 3) &&
      createTask(SensorTask, "SensorTask", 256, 2) &&
      createTask(AlarmTask, "AlarmTask", 192, 2) &&
      createTask(DisplayTask, "DisplayTask", 320, 1);
  if (!created) for (;;) {}
  boardLog("Tasks created; starting scheduler\r\n");
  schedulerStarted = true;
  vTaskStartScheduler();
  schedulerStarted = false;
  boardLog("Startup failed: scheduler returned\r\n");
  for (;;) {}
}

int main() {
  boardInit();
  app_main();
}
