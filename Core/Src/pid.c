#include "pid.h"

// 浮点数也得限制一下，别让它乱飞
static float PID_ClampFloat(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

void PID_Init(PID_TypeDef *pid, float kp, float ki, float kd)
{
    if (pid == NULL)
    {
        return;
    }

    // 先把参数存进去，后面计算就靠它们
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->target = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->integral_limit = 0.0f;
    pid->output_limit = 0.0f;
}

void PID_SetLimit(PID_TypeDef *pid, float output_limit, float integral_limit)
{
    if (pid == NULL)
    {
        return;
    }

    pid->output_limit = output_limit;
    pid->integral_limit = integral_limit;
}

float PID_Calculate(PID_TypeDef *pid, float target, float current)
{
    float error;
    float derivative;
    float output;

    if (pid == NULL)
    {
        return 0.0f;
    }

    // 目标值减当前值，就是现在偏了多少
    error = target - current;
    pid->target = target;
    pid->integral += error;

    if (pid->integral_limit > 0.0f)
    {
        pid->integral = PID_ClampFloat(pid->integral, -pid->integral_limit, pid->integral_limit);
    }

    // 这一次和上一次差多少，用来判断变化快不快
    derivative = error - pid->last_error;
    output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    // 输出也别太夸张，不然车会一下子猛拐
    if (pid->output_limit > 0.0f)
    {
        output = PID_ClampFloat(output, -pid->output_limit, pid->output_limit);
    }

    pid->last_error = error;
    return output;
}
