#include "motor.h"
#include "tim.h"

static uint16_t Motor_ClampSpeed(int speed)
{
    int32_t abs_speed = speed;

    if (abs_speed < 0)
    {
        abs_speed = -abs_speed;
    }

    if (abs_speed > MOTOR_PWM_MAX)
    {
        abs_speed = MOTOR_PWM_MAX;
    }

    return (uint16_t)abs_speed;
}

static void Motor_SetWheel(GPIO_TypeDef *in1_port, uint16_t in1_pin,
                           GPIO_TypeDef *in2_port, uint16_t in2_pin,
                           TIM_HandleTypeDef *htim, uint32_t channel, int speed)
{
    uint16_t pwm = Motor_ClampSpeed(speed);

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

    __HAL_TIM_SET_COMPARE(htim, channel, pwm);
}

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&MOTOR_LEFT_PWM_HANDLE, MOTOR_LEFT_PWM_CHANNEL);
    HAL_TIM_PWM_Start(&MOTOR_RIGHT_PWM_HANDLE, MOTOR_RIGHT_PWM_CHANNEL);
    __HAL_TIM_MOE_ENABLE(&MOTOR_LEFT_PWM_HANDLE);

    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&MOTOR_LEFT_PWM_HANDLE, MOTOR_LEFT_PWM_CHANNEL, 0U);
    __HAL_TIM_SET_COMPARE(&MOTOR_RIGHT_PWM_HANDLE, MOTOR_RIGHT_PWM_CHANNEL, 0U);
}

void Motor_SetSpeed(int left_speed, int right_speed)
{
    Motor_SetWheel(AIN1_GPIO_Port, AIN1_Pin,
                   AIN2_GPIO_Port, AIN2_Pin,
                   &MOTOR_LEFT_PWM_HANDLE, MOTOR_LEFT_PWM_CHANNEL, left_speed);

    Motor_SetWheel(BIN1_GPIO_Port, BIN1_Pin,
                   BIN2_GPIO_Port, BIN2_Pin,
                   &MOTOR_RIGHT_PWM_HANDLE, MOTOR_RIGHT_PWM_CHANNEL, right_speed);
}

void Car_SetSpeed(int16_t left_speed, int16_t right_speed)
{
    Motor_SetSpeed((int)left_speed, (int)right_speed);
}

void Car_Forward(uint16_t speed) {
    Motor_SetSpeed((int)speed, (int)speed);
}

void Car_Backward(uint16_t speed) {
    Motor_SetSpeed(-((int)speed), -((int)speed));
}

void Car_TurnLeft(uint16_t speed) {
    Motor_SetSpeed(-((int)speed), (int)speed);
}

void Car_TurnRight(uint16_t speed) {
    Motor_SetSpeed((int)speed, -((int)speed));
}

void Car_Stop(void) {
    Motor_SetSpeed(0, 0);
}
