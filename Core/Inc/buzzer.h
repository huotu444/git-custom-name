#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

#define BUZZER_PORT  GPIOA
#define BUZZER_PIN   GPIO_PIN_15

// 蜂鸣器控制宏
#define BUZZER_ON()   HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET)
#define BUZZER_OFF()  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET)

void Buzzer_Init(void);
void Buzzer_Alarm(uint16_t duration_ms);

#endif
