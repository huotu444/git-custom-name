#ifndef __PID_H
#define __PID_H

#include "main.h"

typedef struct
{
    float kp;
    float ki;
    float kd;
    float target;
    float last_error;
    float integral;
    float integral_limit;
    float output_limit;
} PID_TypeDef;

void PID_Init(PID_TypeDef *pid, float kp, float ki, float kd);
void PID_SetLimit(PID_TypeDef *pid, float output_limit, float integral_limit);
float PID_Calculate(PID_TypeDef *pid, float target, float current);

#endif
