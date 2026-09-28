#pragma once

#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

void boardInit();
uint16_t boardReadLightAdc();
uint32_t boardMicros();
void boardBuzzer(bool enabled);
void boardLog(const char *message);

constexpr uint16_t DHT_PIN = GPIO_PIN_12;
constexpr uint16_t PIR_PIN = GPIO_PIN_13;
constexpr uint16_t ENCODER_CLK_PIN = GPIO_PIN_1;
constexpr uint16_t ENCODER_DT_PIN = GPIO_PIN_2;
constexpr uint16_t BUZZER_PIN = GPIO_PIN_8;
