#include "system_state.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"

void StateTask(void *) {
  SystemState state = SystemState::ACTIVE;
  TickType_t lastMotion = xTaskGetTickCount();
  for (;;) {
    const EventBits_t bits = xEventGroupWaitBits(systemEvents, EVENT_MOTION,
                                                pdTRUE, pdFALSE, pdMS_TO_TICKS(100));
    const TickType_t now = xTaskGetTickCount();
    const bool motion = (bits & EVENT_MOTION) != 0;
    if (motion) lastMotion = now;
    const SystemState next = evaluateSystemState(
        state, motion, static_cast<uint32_t>(now - lastMotion) * portTICK_PERIOD_MS);
    if (next == state) continue;
    state = next;
    if (state == SystemState::ACTIVE) {
      xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
      boardLog("State: ACTIVE\r\n");
    } else {
      xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
      boardLog("State: INACTIVE\r\n");
    }
  }
}

