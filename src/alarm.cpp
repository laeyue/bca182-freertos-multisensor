#include "alarm.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"

void AlarmTask(void *) {
  AlarmState state = AlarmState::NORMAL;
  bool valid = false;
  for (;;) {
    SensorData data = {};
    if (xQueueReceive(alarmSensorQueue, &data, pdMS_TO_TICKS(100)) == pdTRUE) {
      valid = data.dhtValid;
      const AlarmState next = valid ? evaluateTemperature(data.temperature) : AlarmState::NORMAL;
      if (next != state) {
        state = next;
        boardLog(state == AlarmState::NORMAL ? "Alarm: normal\r\n" :
                 state == AlarmState::HIGH_TEMPERATURE ? "Alarm: high temperature\r\n" :
                                                          "Alarm: low temperature\r\n");
      }
    }
    const bool active = (xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) != 0;
    const bool alarming = active && valid && state != AlarmState::NORMAL;
    boardBuzzer(alarming);
    if (alarming) xEventGroupSetBits(systemEvents, EVENT_ALARM);
    else xEventGroupClearBits(systemEvents, EVENT_ALARM);
  }
}

