/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_menu.c
  * @brief   OLED 任务选择菜单
  ******************************************************************************
  */
/* USER CODE END Header */
#include "oled_menu.h"
#include "button.h"
#include "led.h"
#include "buzzer.h"
#include "i2c.h"

#include <string.h>

#define OLED_WIDTH             128U
#define OLED_PAGES               8U
#define OLED_CONTROL_CMD       0x00U
#define OLED_CONTROL_DATA      0x40U

static uint8_t s_oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t s_selected_task = 1U;
static uint8_t s_menu_dirty = 1U;
static uint16_t s_oled_addr = (0x3CU << 1);

static const uint8_t GLYPH_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t GLYPH_GT[5]    = {0x08, 0x14, 0x22, 0x14, 0x08};
static const uint8_t GLYPH_T[5]     = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t GLYPH_M[5]     = {0x7F, 0x06, 0x18, 0x06, 0x7F};
static const uint8_t GLYPH_a[5]     = {0x20, 0x54, 0x54, 0x54, 0x78};
static const uint8_t GLYPH_s[5]     = {0x48, 0x54, 0x54, 0x54, 0x20};
static const uint8_t GLYPH_k[5]     = {0x7F, 0x08, 0x14, 0x22, 0x41};
static const uint8_t GLYPH_e[5]     = {0x38, 0x54, 0x54, 0x54, 0x18};
static const uint8_t GLYPH_n[5]     = {0x7C, 0x08, 0x04, 0x04, 0x78};
static const uint8_t GLYPH_u[5]     = {0x3C, 0x40, 0x40, 0x20, 0x7C};
static const uint8_t GLYPH_1[5]     = {0x00, 0x42, 0x7F, 0x40, 0x00};
static const uint8_t GLYPH_2[5]     = {0x62, 0x51, 0x49, 0x49, 0x46};
static const uint8_t GLYPH_3[5]     = {0x22, 0x41, 0x49, 0x49, 0x36};
static const uint8_t GLYPH_4[5]     = {0x18, 0x14, 0x12, 0x7F, 0x10};

static const uint8_t *OLED_GetGlyph(char ch)
{
    switch (ch)
    {
        case ' ': return GLYPH_SPACE;
        case '>': return GLYPH_GT;
        case 'T': return GLYPH_T;
        case 'M': return GLYPH_M;
        case 'a': return GLYPH_a;
        case 's': return GLYPH_s;
        case 'k': return GLYPH_k;
        case 'e': return GLYPH_e;
        case 'n': return GLYPH_n;
        case 'u': return GLYPH_u;
        case '1': return GLYPH_1;
        case '2': return GLYPH_2;
        case '3': return GLYPH_3;
        case '4': return GLYPH_4;
        default:  return GLYPH_SPACE;
    }
}

static void OLED_WriteCommand(uint8_t command)
{
    uint8_t buffer[2] = {OLED_CONTROL_CMD, command};
    HAL_I2C_Master_Transmit(&hi2c1, s_oled_addr, buffer, sizeof(buffer), 100);
}

static void OLED_ClearBuffer(void)
{
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

static void OLED_DrawChar(uint8_t page, uint8_t x, char ch)
{
    const uint8_t *glyph = OLED_GetGlyph(ch);
    uint16_t base_index;

    if (page >= OLED_PAGES || x >= (OLED_WIDTH - 6U))
    {
        return;
    }

    base_index = (uint16_t)page * OLED_WIDTH + x;

    for (uint8_t column = 0U; column < 5U; column++)
    {
        s_oled_buffer[base_index + column] = glyph[column];
    }

    s_oled_buffer[base_index + 5U] = 0x00;
}

static void OLED_DrawString(uint8_t page, uint8_t x, const char *text)
{
    while (*text != '\0')
    {
        OLED_DrawChar(page, x, *text);
        x = (uint8_t)(x + 6U);
        text++;

        if (x >= OLED_WIDTH)
        {
            break;
        }
    }
}

static void OLED_Refresh(void)
{
    uint8_t tx_buffer[17];

    for (uint8_t page = 0U; page < OLED_PAGES; page++)
    {
        OLED_WriteCommand((uint8_t)(0xB0U + page));
        OLED_WriteCommand(0x00U);
        OLED_WriteCommand(0x10U);

        tx_buffer[0] = OLED_CONTROL_DATA;

        for (uint8_t offset = 0U; offset < OLED_WIDTH; offset += 16U)
        {
            uint8_t copy_len = (uint8_t)(((OLED_WIDTH - offset) > 16U) ? 16U : (OLED_WIDTH - offset));
            memcpy(&tx_buffer[1], &s_oled_buffer[(uint16_t)page * OLED_WIDTH + offset], copy_len);
            HAL_I2C_Master_Transmit(&hi2c1, s_oled_addr, tx_buffer, (uint16_t)(copy_len + 1U), 100);
        }
    }
}

static void OLED_InitHardware(void)
{
    uint16_t test_addr;

    HAL_Delay(50);

    for (test_addr = (0x3CU << 1); test_addr <= (0x3DU << 1); test_addr = (uint16_t)(test_addr + 2U))
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, test_addr, 2, 100) == HAL_OK)
        {
            s_oled_addr = test_addr;
            break;
        }
    }

    OLED_WriteCommand(0xAEU);
    OLED_WriteCommand(0xD5U);
    OLED_WriteCommand(0x80U);
    OLED_WriteCommand(0xA8U);
    OLED_WriteCommand(0x3FU);
    OLED_WriteCommand(0xD3U);
    OLED_WriteCommand(0x00U);
    OLED_WriteCommand(0x40U);
    OLED_WriteCommand(0x8DU);
    OLED_WriteCommand(0x14U);
    OLED_WriteCommand(0x20U);
    OLED_WriteCommand(0x02U);
    OLED_WriteCommand(0xA1U);
    OLED_WriteCommand(0xC8U);
    OLED_WriteCommand(0xDAU);
    OLED_WriteCommand(0x12U);
    OLED_WriteCommand(0x81U);
    OLED_WriteCommand(0xFFU);
    OLED_WriteCommand(0xD9U);
    OLED_WriteCommand(0xF1U);
    OLED_WriteCommand(0xDBU);
    OLED_WriteCommand(0x40U);
    OLED_WriteCommand(0xA4U);
    OLED_WriteCommand(0xA6U);
    OLED_WriteCommand(0xAFU);
}

static void OLED_RenderMenu(void)
{
    OLED_ClearBuffer();

    OLED_DrawString(0U, 24U, "Task Menu");

    OLED_DrawString(2U, 0U, (s_selected_task == 1U) ? ">" : " ");
    OLED_DrawString(2U, 12U, "Task 1");

    OLED_DrawString(3U, 0U, (s_selected_task == 2U) ? ">" : " ");
    OLED_DrawString(3U, 12U, "Task 2");

    OLED_DrawString(4U, 0U, (s_selected_task == 3U) ? ">" : " ");
    OLED_DrawString(4U, 12U, "Task 3");

    OLED_DrawString(5U, 0U, (s_selected_task == 4U) ? ">" : " ");
    OLED_DrawString(5U, 12U, "Task 4");

    OLED_Refresh();
}

static void OLED_ConfirmTask(void)
{
    LED_ON();
    BUZZER_ON();
    HAL_Delay(100);
    LED_OFF();
    BUZZER_OFF();

    Task_Start(s_selected_task);
}

void OLED_Menu_Init(void)
{
    s_selected_task = 1U;
    s_menu_dirty = 1U;

    OLED_InitHardware();
    OLED_RenderMenu();
    s_menu_dirty = 0U;
}

void OLED_Menu_Process(void)
{
    ButtonEvent_t event;
    uint8_t need_refresh = 0U;

    while (Button_GetEvent(&event) != 0U)
    {
        if (event.event_type == BUTTON_EVENT_SHORT_PRESS)
        {
            if (event.button_id == BUTTON_ID_MODE1)
            {
                s_selected_task = (s_selected_task == 1U) ? 4U : (uint8_t)(s_selected_task - 1U);
                need_refresh = 1U;
            }
            else if (event.button_id == BUTTON_ID_MODE2)
            {
                s_selected_task = (s_selected_task == 4U) ? 1U : (uint8_t)(s_selected_task + 1U);
                need_refresh = 1U;
            }
        }
        else if (event.event_type == BUTTON_EVENT_DOUBLE_CLICK)
        {
            OLED_ConfirmTask();
            need_refresh = 1U;
        }
    }

    if (need_refresh != 0U || s_menu_dirty != 0U)
    {
        OLED_RenderMenu();
        s_menu_dirty = 0U;
    }
}

uint8_t OLED_Menu_GetSelectedTask(void)
{
    return s_selected_task;
}

__weak void Task_Start(uint8_t task_num)
{
    (void)task_num;
}

