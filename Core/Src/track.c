#include "track.h"
#include "bluetooth.h"
#include "motor.h"
#include "pid.h"
#include "usart.h"

#include <string.h>

#define TRACK_BASE_SPEED      360
#define TRACK_MAX_SPEED       999
#define TRACK_MAX_ERROR       7.0f
#define TRACK_FRAME_BUF_LEN   64
#define TRACK_ENABLE_CMD      "$0,0,1#"
#define TRACK_BLACK_LEVEL     0U
#define TRACK_TURN_SIGN       (-1.0f)

static const int8_t s_track_weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};

volatile uint8_t Sensor_Data[8] = {0};
static uint8_t s_track_rx_buf = 0;
static char s_track_frame_buf[TRACK_FRAME_BUF_LEN];
static uint8_t s_track_frame_len = 0;
static float s_track_last_nonzero_error = TRACK_MAX_ERROR;
static PID_TypeDef s_track_pid;

static int16_t Track_ClampSpeed(int16_t speed)
{
    if (speed < 0)
    {
        speed = 0;
    }

    if (speed > TRACK_MAX_SPEED)
    {
        speed = TRACK_MAX_SPEED;
    }

    return speed;
}

void Track_Init(void)
{
    for (uint8_t index = 0; index < 8; index++)
    {
        Sensor_Data[index] = 0;
    }

    s_track_rx_buf = 0;
    memset(s_track_frame_buf, 0, sizeof(s_track_frame_buf));
    s_track_frame_len = 0;
    s_track_last_nonzero_error = 0.0f;

    PID_Init(&s_track_pid, 75.0f, 0.0f, 46.0f);
    PID_SetLimit(&s_track_pid, 350.0f, 0.0f);

    HAL_UART_Transmit(&huart2, (uint8_t *)TRACK_ENABLE_CMD, (uint16_t)(sizeof(TRACK_ENABLE_CMD) - 1U), 100);
    HAL_UART_Receive_IT(&huart2, &s_track_rx_buf, 1);
}

float Track_GetError(void)
{
    int32_t weighted_sum = 0;
    uint8_t active_count = 0;
    float error;

    for (uint8_t index = 0; index < 8; index++)
    {
        if (Sensor_Data[index] == TRACK_BLACK_LEVEL)
        {
            weighted_sum += s_track_weights[index];
            active_count++;
        }
    }

    if (active_count == 0U)
    {
        return s_track_last_nonzero_error;
    }

    error = (float)weighted_sum / (float)active_count;

    if (error > TRACK_MAX_ERROR)
    {
        error = TRACK_MAX_ERROR;
    }
    else if (error < -TRACK_MAX_ERROR)
    {
        error = -TRACK_MAX_ERROR;
    }

    if (error > 0.01f || error < -0.01f)
    {
        s_track_last_nonzero_error = error;
    }

    return error;
}

static void Track_ParseFrame(const char *frame)
{
    const char *cursor;

    if (frame == NULL)
    {
        return;
    }

    if (frame[0] != '$' || frame[1] != 'D')
    {
        return;
    }

    cursor = strchr(frame, ',');
    if (cursor == NULL)
    {
        return;
    }

    for (uint8_t index = 0; index < 8; index++)
    {
        cursor = strchr(cursor + 1, ':');
        if (cursor == NULL)
        {
            return;
        }

        cursor++;
        if (*cursor == '0')
        {
            Sensor_Data[index] = 0;
        }
        else if (*cursor == '1')
        {
            Sensor_Data[index] = 1;
        }
        else
        {
            return;
        }

        if (index < 7U)
        {
            cursor = strchr(cursor, ',');
            if (cursor == NULL)
            {
                return;
            }
        }
    }
}

void Track_FollowLine(void)
{
    float current_error = Track_GetError();
    float turn_out = PID_Calculate(&s_track_pid, 0.0f, current_error) * TRACK_TURN_SIGN;
    int16_t left_speed = (int16_t)((float)TRACK_BASE_SPEED - turn_out);
    int16_t right_speed = (int16_t)((float)TRACK_BASE_SPEED + turn_out);

    left_speed = Track_ClampSpeed(left_speed);
    right_speed = Track_ClampSpeed(right_speed);

    Car_SetSpeed(left_speed, right_speed);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t command = g_bluetooth_rx_buf;

        if (command == 'F' || command == 'B' || command == 'L' || command == 'R' || command == 'S')
        {
            g_bluetooth_cmd = command;
            g_rx_count++;
            g_bluetooth_last_rx_tick = HAL_GetTick();
        }

        HAL_UART_Receive_IT(&huart1, &g_bluetooth_rx_buf, 1);
    }
    else if (huart->Instance == USART2)
    {
        if (s_track_rx_buf == '$')
        {
            s_track_frame_len = 0;
        }

        if (s_track_frame_len < (TRACK_FRAME_BUF_LEN - 1U))
        {
            s_track_frame_buf[s_track_frame_len++] = (char)s_track_rx_buf;
        }

        if (s_track_rx_buf == '#')
        {
            s_track_frame_buf[s_track_frame_len] = '\0';
            Track_ParseFrame(s_track_frame_buf);
            s_track_frame_len = 0;
        }

        HAL_UART_Receive_IT(&huart2, &s_track_rx_buf, 1);
    }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(&huart1, &g_bluetooth_rx_buf, 1);
    }
    else if (huart->Instance == USART2)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(&huart2, &s_track_rx_buf, 1);
    }
}

