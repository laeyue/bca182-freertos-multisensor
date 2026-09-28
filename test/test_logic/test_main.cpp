#include <unity.h>
#include "logic.h"

void test_alarm_below() { TEST_ASSERT_EQUAL(AlarmState::LOW_TEMPERATURE, evaluateTemperature(17.9f)); }
void test_alarm_at_low() { TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(18.0f)); }
void test_alarm_normal() { TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(24.0f)); }
void test_alarm_at_high() { TEST_ASSERT_EQUAL(AlarmState::NORMAL, evaluateTemperature(30.0f)); }
void test_alarm_above() { TEST_ASSERT_EQUAL(AlarmState::HIGH_TEMPERATURE, evaluateTemperature(30.1f)); }
void test_next_middle() { TEST_ASSERT_EQUAL(DisplayMode::LIGHT, nextDisplayMode(DisplayMode::HUMIDITY)); }
void test_next_wrap() { TEST_ASSERT_EQUAL(DisplayMode::TEMPERATURE, nextDisplayMode(DisplayMode::MOTION)); }
void test_previous_middle() { TEST_ASSERT_EQUAL(DisplayMode::HUMIDITY, previousDisplayMode(DisplayMode::LIGHT)); }
void test_previous_wrap() { TEST_ASSERT_EQUAL(DisplayMode::MOTION, previousDisplayMode(DisplayMode::TEMPERATURE)); }
void test_active_before_timeout() { TEST_ASSERT_EQUAL(SystemState::ACTIVE, evaluateSystemState(SystemState::ACTIVE, false, 14999)); }
void test_active_at_timeout() { TEST_ASSERT_EQUAL(SystemState::INACTIVE, evaluateSystemState(SystemState::ACTIVE, false, 15000)); }
void test_inactive_stays() { TEST_ASSERT_EQUAL(SystemState::INACTIVE, evaluateSystemState(SystemState::INACTIVE, false, 60000)); }
void test_motion_reactivates() { TEST_ASSERT_EQUAL(SystemState::ACTIVE, evaluateSystemState(SystemState::INACTIVE, true, 60000)); }
void test_light_dark() { TEST_ASSERT_EQUAL(0, lightPercentFromAdc(4095)); }
void test_light_bright() { TEST_ASSERT_EQUAL(100, lightPercentFromAdc(0)); }

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_alarm_below); RUN_TEST(test_alarm_at_low); RUN_TEST(test_alarm_normal);
  RUN_TEST(test_alarm_at_high); RUN_TEST(test_alarm_above);
  RUN_TEST(test_next_middle); RUN_TEST(test_next_wrap);
  RUN_TEST(test_previous_middle); RUN_TEST(test_previous_wrap);
  RUN_TEST(test_active_before_timeout); RUN_TEST(test_active_at_timeout);
  RUN_TEST(test_inactive_stays); RUN_TEST(test_motion_reactivates);
  RUN_TEST(test_light_dark); RUN_TEST(test_light_bright);
  return UNITY_END();
}
