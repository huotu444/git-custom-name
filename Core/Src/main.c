/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "buzzer.h"
#include "button.h"
#include "motor.h"
#include "track.h"
#include "oled_menu.h"
#include "mpu6050.h"

#include <stdio.h>

extern float Car_Yaw;
extern System_StateTypeDef System_State;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TARGET_PULSE_100CM  7036L
#define BASE_SPEED          400
#define KP_YAW              15.0f
#define TASK1_FINISH_MS     500U

#define MIN_DIST_A2B        1500L  // 盲走必须超过此距离，才允许寻找B点黑线
#define MIN_DIST_B2C        3000L  // 循迹必须超过此距离，才允许检测C点出弯(全白)
#define MIN_DIST_C2D        1500L  // 盲走必须超过此距离，才允许寻找D点黑线
#define YAW_COMPENSATE        15.0f // C点出弯航向补偿角

#define T2_PREPARE_MS       1000U
#define NODE_SIGNAL_MS       500U
#define T2_STEP_PREPARE    ((uint8_t)0xFFU)
#define T2_BLIND_BASE_SPEED  360
#define T2_BLIND_KP         15.0f
#define T2_BLIND_MAX_TURN  260.0f
#define T2_FINISH_DIST      5200L  // D->A 至少到后段后才允许按全白结束
#define T2_FINISH_FAILSAFE_DIST 9000L // 极端情况下的超长距离保险停止，避免无限绕行
#define T2_FINISH_WHITE_MS   300U  // 全白需持续更久，避免弯道瞬时丢线误停

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
long Total_EncL = 0L;
long Total_EncR = 0L;
char Total_EncL_Text[16] = "0";
char Total_EncR_Text[16] = "0";
static uint8_t s_active_task = 0U;
static uint32_t s_mpu_update_tick = 0U;
static uint32_t s_encoder_update_tick = 0U;
static uint16_t s_prev_enc_l = 0U;
static uint16_t s_prev_enc_r = 0U;
static uint8_t s_task1_started = 0U;
static uint8_t s_task1_finishing = 0U;
static uint32_t s_task1_finish_tick = 0U;
static uint8_t s_node_signal_active = 0U;
static uint32_t s_node_signal_start_tick = 0U;
static uint8_t s_t2_start_request = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Read_Encoders(void);
void Run_Task_1(void);
void Run_Task_2(void);
void Run_Task_3(void);
void Run_Task_4(void);
void Trigger_Node_Signal(void);
void Node_Signal_Process(uint32_t now);
static void Task2_ResetMotion(uint32_t now);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  LED_Init();
  Buzzer_Init();
  Button_Init();
  Motor_Init();
  OLED_Menu_Init();
  Track_Init();

  /* TIM2 = left encoder, TIM3 = right encoder; verify CubeMX encoder mode and ARR = 65535. */
  if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);
  Total_EncL = 0L;
  Total_EncR = 0L;
  s_prev_enc_l = 0U;
  s_prev_enc_r = 0U;
  (void)sprintf(Total_EncL_Text, "%ld", Total_EncL);
  (void)sprintf(Total_EncR_Text, "%ld", Total_EncR);

  MPU_Init();
  s_encoder_update_tick = HAL_GetTick();
  s_mpu_update_tick = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    {
      uint32_t now = HAL_GetTick();

      while ((uint32_t)(now - s_encoder_update_tick) >= 5U)
      {
        Read_Encoders();
        s_encoder_update_tick = (uint32_t)(s_encoder_update_tick + 5U);
      }

      while ((uint32_t)(now - s_mpu_update_tick) >= 10U)
      {
        MPU_Update_Yaw(0.01f);
        s_mpu_update_tick = (uint32_t)(s_mpu_update_tick + 10U);
      }

      Node_Signal_Process(now);

      if (System_State == PAGE_DASHBOARD)
      {
        switch (s_active_task)
        {
          case 1U:
            Run_Task_1();
            break;

          case 2U:
            Run_Task_2();
            break;

          case 3U:
            Run_Task_3();
            break;

          case 4U:
            Run_Task_4();
            break;

          default:
            break;
        }
      }
    }

    Button_Scan();
    OLED_Menu_Process();
    HAL_Delay(1);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void Task_Start(uint8_t task_num)
{
  if (task_num >= 1U && task_num <= 4U)
  {
    s_active_task = task_num;
  }
  else
  {
    s_active_task = 0U;
  }

  if (task_num == 2U)
  {
    s_t2_start_request = 1U;
  }
  else
  {
    s_t2_start_request = 0U;
  }
}

void Trigger_Node_Signal(void)
{
  s_node_signal_active = 1U;
  s_node_signal_start_tick = HAL_GetTick();
  LED_ON();
  BUZZER_ON();
}

void Node_Signal_Process(uint32_t now)
{
  if (s_node_signal_active != 0U && (uint32_t)(now - s_node_signal_start_tick) >= NODE_SIGNAL_MS)
  {
    LED_OFF();
    BUZZER_OFF();
    s_node_signal_active = 0U;
  }
}

static void Task2_ResetMotion(uint32_t now)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);

  Total_EncL = 0L;
  Total_EncR = 0L;
  s_prev_enc_l = 0U;
  s_prev_enc_r = 0U;

  (void)sprintf(Total_EncL_Text, "%ld", Total_EncL);
  (void)sprintf(Total_EncR_Text, "%ld", Total_EncR);

  s_encoder_update_tick = now;
  s_mpu_update_tick = now;
}

void Run_Task_1(void)
{
  uint32_t now = HAL_GetTick();
  long average_pulse;
  float error_yaw;
  float turn_out;
  int left_speed;
  int right_speed;

  if (s_task1_started == 0U)
  {
    uint16_t current_enc_l;
    uint16_t current_enc_r;

    s_task1_started = 1U;
    s_task1_finishing = 0U;
    s_task1_finish_tick = 0U;

    Total_EncL = 0L;
    Total_EncR = 0L;
    Car_Yaw = 0.0f;

    current_enc_l = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    current_enc_r = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    s_prev_enc_l = current_enc_l;
    s_prev_enc_r = current_enc_r;

    s_encoder_update_tick = now;
    s_mpu_update_tick = now;

    (void)sprintf(Total_EncL_Text, "%ld", Total_EncL);
    (void)sprintf(Total_EncR_Text, "%ld", Total_EncR);
  }

  if (s_task1_finishing != 0U)
  {
    Motor_SetSpeed(0, 0);

    if ((uint32_t)(now - s_task1_finish_tick) < TASK1_FINISH_MS)
    {
      LED_ON();
      BUZZER_ON();
      return;
    }

    LED_OFF();
    BUZZER_OFF();
    s_active_task = 0U;
    s_task1_started = 0U;
    s_task1_finishing = 0U;
    s_task1_finish_tick = 0U;
    OLED_Menu_Init();
    now = HAL_GetTick();
    s_encoder_update_tick = now;
    s_mpu_update_tick = now;
    return;
  }

  average_pulse = (Total_EncL + Total_EncR) / 2L;

  if (average_pulse >= TARGET_PULSE_100CM)
  {
    Motor_SetSpeed(0, 0);
    s_task1_finishing = 1U;
    s_task1_finish_tick = now;
    LED_ON();
    BUZZER_ON();
    return;
  }

  error_yaw = 0.0f - Car_Yaw;
  turn_out = KP_YAW * error_yaw;
  left_speed = (int)((float)BASE_SPEED + turn_out);
  right_speed = (int)((float)BASE_SPEED - turn_out);

  Motor_SetSpeed(left_speed, right_speed);
}

__weak void Run_Task_2(void)
{
  uint32_t now = HAL_GetTick();
  long average_pulse;
  float error_yaw;
  float turn_out;
  int left_speed;
  int right_speed;
  static uint8_t t2_step = 0U;
  static float T2_Target_Yaw = 0.0f;
  static uint8_t t2_finish_pending = 0U;
  static uint8_t t2_finish_white_active = 0U;
  static uint32_t t2_finish_white_tick = 0U;
  static uint32_t t2_prepare_tick = 0U;

  if (s_t2_start_request != 0U)
  {
    s_t2_start_request = 0U;
    t2_finish_pending = 0U;
    t2_finish_white_active = 0U;
    t2_finish_white_tick = 0U;
    t2_step = T2_STEP_PREPARE;
    T2_Target_Yaw = 0.0f;

    Trigger_Node_Signal();
    Car_Yaw = 0.0f;
    Task2_ResetMotion(now);
    Car_SetSpeed(0, 0);
    t2_prepare_tick = now;
    return;
  }

  if (t2_finish_pending != 0U)
  {
    Car_SetSpeed(0, 0);

    if (s_node_signal_active == 0U)
    {
      s_active_task = 0U;
      t2_finish_pending = 0U;
      t2_step = 0U;
      T2_Target_Yaw = 0.0f;
      s_t2_start_request = 0U;

      OLED_Menu_Init();
      now = HAL_GetTick();
      s_encoder_update_tick = now;
      s_mpu_update_tick = now;
    }

    return;
  }

  switch (t2_step)
  {
    case T2_STEP_PREPARE:
      Car_SetSpeed(0, 0);

      if ((uint32_t)(now - t2_prepare_tick) >= T2_PREPARE_MS)
      {
        t2_step = 1U;
      }
      break;

    case 1U:
      average_pulse = (Total_EncL + Total_EncR) / 2L;
      error_yaw = 0.0f - Car_Yaw;
      turn_out = T2_BLIND_KP * error_yaw;

      if (turn_out > T2_BLIND_MAX_TURN)
      {
        turn_out = T2_BLIND_MAX_TURN;
      }
      else if (turn_out < -T2_BLIND_MAX_TURN)
      {
        turn_out = -T2_BLIND_MAX_TURN;
      }

      left_speed = (int)((float)T2_BLIND_BASE_SPEED + turn_out);
      right_speed = (int)((float)T2_BLIND_BASE_SPEED - turn_out);
      Car_SetSpeed(left_speed, right_speed);

      if ((average_pulse > MIN_DIST_A2B) && (Track_IsLostLine() == 0U))
      {
        Trigger_Node_Signal();
        Task2_ResetMotion(now);
        t2_step = 2U;
      }
      break;

    case 2U:
      Track_FollowLine();
      average_pulse = (Total_EncL + Total_EncR) / 2L;

      if ((average_pulse > MIN_DIST_B2C) && (Track_IsLostLine() == 1U))
      {
        Trigger_Node_Signal();
        T2_Target_Yaw = Car_Yaw + YAW_COMPENSATE;
        Task2_ResetMotion(now);
        t2_step = 3U;
      }
      break;

    case 3U:
      average_pulse = (Total_EncL + Total_EncR) / 2L;
      error_yaw = T2_Target_Yaw - Car_Yaw;
      turn_out = T2_BLIND_KP * error_yaw;

      if (turn_out > T2_BLIND_MAX_TURN)
      {
        turn_out = T2_BLIND_MAX_TURN;
      }
      else if (turn_out < -T2_BLIND_MAX_TURN)
      {
        turn_out = -T2_BLIND_MAX_TURN;
      }

      left_speed = (int)((float)T2_BLIND_BASE_SPEED + turn_out);
      right_speed = (int)((float)T2_BLIND_BASE_SPEED - turn_out);
      Car_SetSpeed(left_speed, right_speed);

      if ((average_pulse > MIN_DIST_C2D) && (Track_IsLostLine() == 0U))
      {
        Trigger_Node_Signal();
        Task2_ResetMotion(now);
        t2_finish_white_active = 0U;
        t2_finish_white_tick = 0U;
        t2_step = 4U;
      }
      break;

    case 4U:
      Track_FollowLine();
      average_pulse = (Total_EncL + Total_EncR) / 2L;

      if ((average_pulse > T2_FINISH_DIST) && (Track_IsLostLine() == 1U))
      {
        if (t2_finish_white_active == 0U)
        {
          t2_finish_white_active = 1U;
          t2_finish_white_tick = now;
        }
        else if ((uint32_t)(now - t2_finish_white_tick) >= T2_FINISH_WHITE_MS)
        {
          Car_SetSpeed(0, 0);
          Trigger_Node_Signal();
          t2_finish_pending = 1U;
        }
      }
      else
      {
        t2_finish_white_active = 0U;

        if (average_pulse > T2_FINISH_FAILSAFE_DIST)
        {
          Car_SetSpeed(0, 0);
          Trigger_Node_Signal();
          t2_finish_pending = 1U;
        }
      }
      break;

    default:
      t2_step = T2_STEP_PREPARE;
      t2_finish_white_active = 0U;
      t2_finish_white_tick = 0U;
      Trigger_Node_Signal();
      Car_Yaw = 0.0f;
      Task2_ResetMotion(now);
      Car_SetSpeed(0, 0);
      t2_prepare_tick = now;
      break;
  }
}

__weak void Run_Task_3(void)
{
}

__weak void Run_Task_4(void)
{
}

void Read_Encoders(void)
{
  uint16_t enc_l_now;
  uint16_t enc_r_now;
  int16_t enc_l_delta;
  int16_t enc_r_delta;

  enc_l_now = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
  enc_r_now = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);

  enc_l_delta = (int16_t)(enc_l_now - s_prev_enc_l);
  enc_r_delta = (int16_t)(enc_r_now - s_prev_enc_r);

  Total_EncL += (long)(-enc_l_delta);
  Total_EncR += (long)(enc_r_delta);

  s_prev_enc_l = enc_l_now;
  s_prev_enc_r = enc_r_now;

  (void)sprintf(Total_EncL_Text, "%ld", Total_EncL);
  (void)sprintf(Total_EncR_Text, "%ld", Total_EncR);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
