#include "board.h"
#include "rtos_objects.h"
#include "task.h"
#include <cstring>

ADC_HandleTypeDef hadc1 = {};
I2C_HandleTypeDef hi2c1 = {};
UART_HandleTypeDef huart1 = {};
TIM_HandleTypeDef htim2 = {};
TIM_HandleTypeDef htim4 = {};

static void initClocks() {
  RCC_OscInitTypeDef oscillator = {};
  oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  oscillator.HSIState = RCC_HSI_ON;
  oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  oscillator.PLL.PLLState = RCC_PLL_ON;
  oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  oscillator.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) while (true) {}

  RCC_ClkInitTypeDef clocks = {};
  clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clocks.APB1CLKDivider = RCC_HCLK_DIV2;
  clocks.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK) while (true) {}
}

void boardInit() {
  // Wokwi maps the firmware vectors at flash base but does not alias them at 0x0.
  SCB->VTOR = FLASH_BASE;
  __DSB();
  __ISB();
  HAL_Init();
  SCB->VTOR = FLASH_BASE;
  __DSB();
  __ISB();
  initClocks();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {};
  gpio.Pin = DHT_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = BUZZER_PIN;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = PIR_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &gpio);

  gpio.Pin = ENCODER_CLK_PIN;
  gpio.Mode = GPIO_MODE_IT_FALLING;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpio);
  gpio.Pin = ENCODER_DT_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOA, &gpio);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_AF_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = GPIO_PIN_9;
  gpio.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOA, &gpio);
  gpio.Pin = GPIO_PIN_0;
  gpio.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &gpio);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) while (true) {}

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) while (true) {}

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) while (true) {}
  ADC_ChannelConfTypeDef channel = {};
  channel.Channel = ADC_CHANNEL_0;
  channel.Rank = ADC_REGULAR_RANK_1;
  channel.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &channel) != HAL_OK) while (true) {}
  HAL_ADCEx_Calibration_Start(&hadc1);

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = (SystemCoreClock / 1000000U) - 1U;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffU;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK || HAL_TIM_Base_Start(&htim2) != HAL_OK)
    while (true) {}

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = (SystemCoreClock / 1000000U) - 1U;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1999U;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) while (true) {}
  TIM_OC_InitTypeDef pulse = {};
  pulse.OCMode = TIM_OCMODE_PWM1;
  pulse.Pulse = 1000U;
  pulse.OCPolarity = TIM_OCPOLARITY_HIGH;
  pulse.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &pulse, TIM_CHANNEL_3) != HAL_OK) while (true) {}
}

uint16_t boardReadLightAdc() {
  if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0;
  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) return 0;
  const uint16_t value = static_cast<uint16_t>(HAL_ADC_GetValue(&hadc1));
  HAL_ADC_Stop(&hadc1);
  return value;
}

uint32_t boardMicros() {
  return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

void boardBuzzer(bool enabled) {
  static bool sounding = false;
  if (enabled == sounding) return;
  if (enabled) HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  else HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
  sounding = enabled;
}

void boardLog(const char *message) {
  const TickType_t timeout = xTaskGetSchedulerState() == taskSCHEDULER_RUNNING
      ? pdMS_TO_TICKS(100) : 0;
  if (!serialMutex || xSemaphoreTake(serialMutex, timeout) != pdTRUE) return;
  HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(const_cast<char *>(message)),
                    static_cast<uint16_t>(std::strlen(message)), 100);
  xSemaphoreGive(serialMutex);
}
