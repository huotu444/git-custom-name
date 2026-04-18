#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

// 电机引脚定义 (严格按照你的硬件配置)
// 左轮 (电机A)：AIN1 -> PB15, AIN2 -> PB14, PWMA -> PA8 (TIM1_CH1)
#define MOTOR_A_IN1_PORT  GPIOB
#define MOTOR_A_IN1_PIN   GPIO_PIN_15
#define MOTOR_A_IN2_PORT  GPIOB
#define MOTOR_A_IN2_PIN   GPIO_PIN_14

// 右轮 (电机B)：BIN1 -> PB13, BIN2 -> PB12, PWMB -> PA11 (TIM1_CH4)
#define MOTOR_B_IN1_PORT  GPIOB
#define MOTOR_B_IN1_PIN   GPIO_PIN_13
#define MOTOR_B_IN2_PORT  GPIOB
#define MOTOR_B_IN2_PIN   GPIO_PIN_12

// 运动控制函数声明 (速度范围 0-1000)
void Motor_Init(void);
void Car_SetSpeed(int16_t left_speed, int16_t right_speed);
void Car_Forward(uint16_t speed);
void Car_Backward(uint16_t speed);
void Car_TurnLeft(uint16_t speed);
void Car_TurnRight(uint16_t speed);
void Car_Stop(void);

#endif
