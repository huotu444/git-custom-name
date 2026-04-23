#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

/*
 * 电机方向引脚占位宏。
 * 如果 CubeMX 已经生成了 AIN1/AIN2/BIN1/BIN2 的 GPIO 宏，这里会自动优先使用 CubeMX 的定义。
 * 否则你可以直接把下面的 GPIO 端口和 Pin 改成你的真实硬件定义。
 */
#ifndef AIN1_GPIO_Port
#define AIN1_GPIO_Port GPIOB
#endif
#ifndef AIN1_Pin
#define AIN1_Pin GPIO_PIN_15
#endif

#ifndef AIN2_GPIO_Port
#define AIN2_GPIO_Port GPIOB
#endif
#ifndef AIN2_Pin
#define AIN2_Pin GPIO_PIN_14
#endif

#ifndef BIN1_GPIO_Port
#define BIN1_GPIO_Port GPIOB
#endif
#ifndef BIN1_Pin
#define BIN1_Pin GPIO_PIN_13
#endif

#ifndef BIN2_GPIO_Port
#define BIN2_GPIO_Port GPIOB
#endif
#ifndef BIN2_Pin
#define BIN2_Pin GPIO_PIN_12
#endif

#ifndef MOTOR_LEFT_PWM_HANDLE
#define MOTOR_LEFT_PWM_HANDLE htim1
#endif
#ifndef MOTOR_LEFT_PWM_CHANNEL
#define MOTOR_LEFT_PWM_CHANNEL TIM_CHANNEL_1
#endif

#ifndef MOTOR_RIGHT_PWM_HANDLE
#define MOTOR_RIGHT_PWM_HANDLE htim1
#endif
#ifndef MOTOR_RIGHT_PWM_CHANNEL
#define MOTOR_RIGHT_PWM_CHANNEL TIM_CHANNEL_4
#endif

#define MOTOR_PWM_MAX 1000

/* 运动控制函数声明 */
void Motor_Init(void);
void Motor_SetSpeed(int left_speed, int right_speed);

/* 兼容旧接口，巡线等现有代码可以继续直接调用 */
void Car_SetSpeed(int16_t left_speed, int16_t right_speed);
void Car_Forward(uint16_t speed);
void Car_Backward(uint16_t speed);
void Car_TurnLeft(uint16_t speed);
void Car_TurnRight(uint16_t speed);
void Car_Stop(void);

#endif
