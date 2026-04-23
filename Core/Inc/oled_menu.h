/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_menu.h
  * @brief   OLED 任务选择菜单
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __OLED_MENU_H
#define __OLED_MENU_H

#include "main.h"

typedef enum
{
  PAGE_MENU = 0U,
  PAGE_DASHBOARD = 1U
} System_StateTypeDef;

extern System_StateTypeDef System_State;

void OLED_Menu_Init(void);
void OLED_Menu_Process(void);
void OLED_Clear(void);
uint8_t OLED_Menu_GetSelectedTask(void);
void Task_Start(uint8_t task_num);

#endif
