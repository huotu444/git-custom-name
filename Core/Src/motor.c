#include "motor.h"
#include "tim.h"

static uint16_t Motor_ClampSpeed(int16_t speed)
{
    if (speed < 0)
    {
        speed = (int16_t)(-speed);
    }

    if (speed > 999)
    {
        speed = 999;
    }

    return (uint16_t)speed;
}

static void Motor_SetWheel(GPIO_TypeDef *in1_port, uint16_t in1_pin,
                           GPIO_TypeDef *in2_port, uint16_t in2_pin,
                           uint32_t channel, int16_t speed)
{
    if (speed > 0)
    {
        HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
    }
    else if (speed < 0)
    {
        HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
    }

    __HAL_TIM_SET_COMPARE(&htim1, channel, Motor_ClampSpeed(speed));
}

void Motor_Init(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // PWMA (PA8)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // PWMB (PA11)
    __HAL_TIM_MOE_ENABLE(&htim1);             // 高级定时器主输出使能
}

void Car_SetSpeed(int16_t left_speed, int16_t right_speed)
{
    Motor_SetWheel(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN,
                   MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN,
                   TIM_CHANNEL_1, left_speed);

    Motor_SetWheel(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN,
                   MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN,
                   TIM_CHANNEL_4, right_speed);
}

void Car_Forward(uint16_t speed) {
    Car_SetSpeed((int16_t)speed, (int16_t)speed);
}

void Car_Backward(uint16_t speed) {
    Car_SetSpeed((int16_t)(-((int16_t)speed)), (int16_t)(-((int16_t)speed)));
}

void Car_TurnLeft(uint16_t speed) {
    Car_SetSpeed((int16_t)(-((int16_t)speed)), (int16_t)speed);
}

void Car_TurnRight(uint16_t speed) {
    Car_SetSpeed((int16_t)speed, (int16_t)(-((int16_t)speed)));
}

void Car_Stop(void) {
    Car_SetSpeed(0, 0);
}
