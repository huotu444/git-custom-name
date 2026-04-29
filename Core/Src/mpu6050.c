#include "mpu6050.h"
#include "i2c.h"

#define MPU6050_REG_SMPLRT_DIV        0x19U
#define MPU6050_REG_CONFIG            0x1AU
#define MPU6050_REG_GYRO_CONFIG       0x1BU
#define MPU6050_REG_PWR_MGMT_1       0x6BU
#define MPU6050_REG_GYRO_ZOUT_H      0x47U
#define MPU6050_I2C_TIMEOUT          100U
#define MPU6050_CALIBRATION_SAMPLES  100U

float Car_Yaw = 0.0f;

static int32_t s_gyro_z_offset = 0;

static HAL_StatusTypeDef MPU_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1,
                             MPU6050_I2C_ADDR,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1U,
                             MPU6050_I2C_TIMEOUT);
}

static HAL_StatusTypeDef MPU_ReadRegs(uint8_t reg, uint8_t *buffer, uint16_t length)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            MPU6050_I2C_ADDR,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            buffer,
                            length,
                            MPU6050_I2C_TIMEOUT);
}

static HAL_StatusTypeDef MPU_Read_GyroZ_Raw(int16_t *raw_z)
{
    uint8_t buffer[2];

    if (raw_z == NULL)
    {
        return HAL_ERROR;
    }

    if (MPU_ReadRegs(MPU6050_REG_GYRO_ZOUT_H, buffer, sizeof(buffer)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *raw_z = (int16_t)(((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1]);
    return HAL_OK;
}

void MPU_Init(void)
{
    int32_t sum = 0;
    uint16_t valid_count = 0U;
    uint16_t attempts = 0U;
    int16_t raw_z = 0;

    Car_Yaw = 0.0f;
    s_gyro_z_offset = 0;

    HAL_Delay(100);

    (void)MPU_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00U);
    HAL_Delay(10);
    (void)MPU_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07U);
    (void)MPU_WriteReg(MPU6050_REG_CONFIG, 0x06U);
    (void)MPU_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00U);
    HAL_Delay(10);

    HAL_Delay(500);

    while ((valid_count < MPU6050_CALIBRATION_SAMPLES) && (attempts < (MPU6050_CALIBRATION_SAMPLES * 2U)))
    {
        attempts++;

        if (MPU_Read_GyroZ_Raw(&raw_z) == HAL_OK)
        {
            sum += raw_z;
            valid_count++;
        }

        HAL_Delay(2);
    }

    if (valid_count != 0U)
    {
        s_gyro_z_offset = sum / (int32_t)valid_count;
    }
    else
    {
        s_gyro_z_offset = 0;
    }
}

void MPU_Update_Yaw(float dt)
{
    int16_t raw_z = 0;
    int32_t corrected_z;
    float gyro_rate;

    if (dt <= 0.0f)
    {
        return;
    }

    if (MPU_Read_GyroZ_Raw(&raw_z) != HAL_OK)
    {
        return;
    }

    corrected_z = (int32_t)raw_z - s_gyro_z_offset;
    gyro_rate = (float)corrected_z / MPU6050_GYRO_SENSITIVITY;

    if (gyro_rate > -0.3f && gyro_rate < 0.3f)
    {
        gyro_rate = 0.0f;
    }

    Car_Yaw += gyro_rate * dt;
}

float MPU_GetYaw(void)
{
    return Car_Yaw;
}
