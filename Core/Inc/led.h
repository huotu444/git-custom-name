/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    led.h
  * @brief   LED 控制模块，PA12 用于确认反馈
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __LED_H
#define __LED_H

#include "main.h"

#define LED_PORT  GPIOA
#define LED_PIN   GPIO_PIN_12

#define LED_ON()      HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_OFF()     HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_TOGGLE()  HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

void LED_Init(void);
void LED_Flash(uint16_t duration_ms);

#endif
