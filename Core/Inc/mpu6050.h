#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

#define MPU6050_I2C_ADDR              (0x68U << 1)
#define MPU6050_GYRO_SENSITIVITY      131.0f

extern float Car_Yaw;

void MPU_Init(void);
void MPU_Update_Yaw(float dt);
float MPU_GetYaw(void);

#endif
