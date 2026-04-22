/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    button.h
  * @brief   按键扫描模块，支持短按和双击
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __BUTTON_H
#define __BUTTON_H

#include "main.h"

typedef enum
{
    BUTTON_ID_MODE1 = 0,
    BUTTON_ID_MODE2 = 1
} ButtonIdTypeDef;

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_DOUBLE_CLICK
} ButtonEventTypeDef;

typedef struct
{
    ButtonIdTypeDef button_id;
    ButtonEventTypeDef event_type;
} ButtonEvent_t;

void Button_Init(void);
void Button_Scan(void);
uint8_t Button_GetEvent(ButtonEvent_t *event);

#endif
