#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "main.h"

// 引脚定义：Trig -> PA4, Echo -> PB7
#define TRIG_PORT  GPIOA
#define TRIG_PIN   GPIO_PIN_4
#define ECHO_PORT  GPIOB
#define ECHO_PIN   GPIO_PIN_7

float Ultrasonic_GetDistance(void);

#endif
