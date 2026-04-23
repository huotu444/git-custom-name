/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_menu.c
  * @brief   OLED 页面状态机，支持主菜单与运行看板切换
  ******************************************************************************
  */
/* USER CODE END Header */
#include "oled_menu.h"
#include "button.h"
#include "led.h"
#include "buzzer.h"
#include "i2c.h"
#include "mpu6050.h"
#include "oledfont.h"

#include <stdio.h>
#include <string.h>

#define OLED_WIDTH                 128U
#define OLED_PAGES                   8U
#define OLED_CONTROL_CMD           0x00U
#define OLED_CONTROL_DATA          0x40U
#define OLED_MENU_LINES             4U
#define OLED_FEEDBACK_MS          100U


System_StateTypeDef System_State = PAGE_MENU;

static uint8_t s_oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t s_selected_task = 1U;
static uint8_t s_running_task = 1U;
static uint8_t s_menu_dirty = 1U;
static uint8_t s_dashboard_dirty = 1U;
static uint16_t s_oled_addr = (0x3CU << 1);
static uint8_t s_refresh_pending = 0U;
static uint8_t s_refresh_page = 0U;
static uint8_t s_refresh_end_page = 3U;
static uint8_t s_feedback_active = 0U;
static uint32_t s_feedback_off_tick = 0U;
static uint32_t s_dashboard_refresh_tick = 0U;

typedef struct
{
    char ch;
    uint8_t glyph[5];
} OledGlyphEntry;

static const OledGlyphEntry s_ascii_font[] =
{
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {'>', {0x08, 0x14, 0x22, 0x14, 0x08}},
    {'[', {0x00, 0x7F, 0x41, 0x41, 0x00}},
    {']', {0x00, 0x41, 0x41, 0x7F, 0x00}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {'A', {0x7E, 0x11, 0x11, 0x7E, 0x00}},
    {'C', {0x3E, 0x41, 0x41, 0x22, 0x00}},
    {'D', {0x7F, 0x41, 0x41, 0x3E, 0x00}},
    {'E', {0x7F, 0x49, 0x49, 0x41, 0x00}},
    {'G', {0x3E, 0x41, 0x49, 0x7A, 0x00}},
    {'I', {0x41, 0x7F, 0x41, 0x00, 0x00}},
    {'K', {0x7F, 0x08, 0x14, 0x63, 0x00}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x00}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x06, 0x18, 0x7F, 0x00}},
    {'R', {0x7F, 0x09, 0x19, 0x66, 0x00}},
    {'S', {0x46, 0x49, 0x49, 0x31, 0x00}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x3F, 0x00}},
    {'W', {0x7F, 0x20, 0x18, 0x20, 0x7F}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}}
};

static const uint8_t s_space_glyph[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct
{
    const uint8_t *glyph;
    uint8_t width;
} OledGlyphRef;

static OledGlyphRef OLED_GetGlyph(char ch)
{
    OledGlyphRef glyph_ref;

    if (ch >= '0' && ch <= '9')
    {
        glyph_ref.glyph = F6x8_Num[(uint8_t)(ch - '0')];
        glyph_ref.width = 6U;
        return glyph_ref;
    }

    if (ch >= 'a' && ch <= 'z')
    {
        ch = (char)(ch - ('a' - 'A'));
    }

    for (uint8_t index = 0U; index < (uint8_t)(sizeof(s_ascii_font) / sizeof(s_ascii_font[0])); index++)
    {
        if (s_ascii_font[index].ch == ch)
        {
            glyph_ref.glyph = s_ascii_font[index].glyph;
            glyph_ref.width = 5U;
            return glyph_ref;
        }
    }

    glyph_ref.glyph = s_space_glyph;
    glyph_ref.width = 5U;
    return glyph_ref;
}

static void OLED_WriteCommand(uint8_t command)
{
    uint8_t buffer[2] = {OLED_CONTROL_CMD, command};
    HAL_I2C_Master_Transmit(&hi2c1, s_oled_addr, buffer, sizeof(buffer), 100);
}

static void OLED_SetCursor(uint8_t page, uint8_t x)
{
    uint8_t command_buffer[4];

    x = (uint8_t)(x + 2U);

    command_buffer[0] = OLED_CONTROL_CMD;
    command_buffer[1] = (uint8_t)(0xB0U + page);
    command_buffer[2] = (uint8_t)(0x00U | (x & 0x0FU));
    command_buffer[3] = (uint8_t)(0x10U | (uint8_t)(x >> 4));
    HAL_I2C_Master_Transmit(&hi2c1, s_oled_addr, command_buffer, sizeof(command_buffer), 100);
}

static void OLED_RefreshPage(uint8_t page)
{
    uint8_t tx_buffer[OLED_WIDTH + 1U];

    tx_buffer[0] = OLED_CONTROL_DATA;
    memcpy(&tx_buffer[1], &s_oled_buffer[(uint16_t)page * OLED_WIDTH], OLED_WIDTH);
    HAL_I2C_Master_Transmit(&hi2c1, s_oled_addr, tx_buffer, sizeof(tx_buffer), 100);
}

static void OLED_RequestRefresh(uint8_t end_page)
{
    if (end_page >= OLED_PAGES)
    {
        end_page = (uint8_t)(OLED_PAGES - 1U);
    }

    s_refresh_pending = 1U;
    s_refresh_page = 0U;

    if (end_page > s_refresh_end_page || s_refresh_end_page >= OLED_PAGES)
    {
        s_refresh_end_page = end_page;
    }
    else
    {
        s_refresh_end_page = end_page;
    }
}

static void OLED_RequestSinglePageRefresh(uint8_t page)
{
    if (page >= OLED_PAGES)
    {
        return;
    }

    s_refresh_pending = 1U;
    s_refresh_page = page;
    s_refresh_end_page = page;
}

static void OLED_ClearBuffer(void)
{
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

void OLED_Clear(void)
{
    OLED_ClearBuffer();
}

static void OLED_DrawChar(uint8_t page, uint8_t x, char ch)
{
    OledGlyphRef glyph;
    uint16_t base_index;

    if (page >= OLED_PAGES || x >= (OLED_WIDTH - 6U))
    {
        return;
    }

    glyph = OLED_GetGlyph(ch);
    base_index = (uint16_t)page * OLED_WIDTH + x;


    for (uint8_t column = 0U; column < glyph.width; column++)
    {
        s_oled_buffer[base_index + column] = glyph.glyph[column];
    }

    if (glyph.width < 6U)
    {
        s_oled_buffer[base_index + glyph.width] = 0x00;
    }
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

static void OLED_RefreshService(void)
{
    if (s_refresh_pending == 0U)
    {
        return;
    }

    if (s_refresh_page > s_refresh_end_page)
    {
        s_refresh_pending = 0U;
        return;
    }

    OLED_SetCursor(s_refresh_page, 0U);
    OLED_RefreshPage(s_refresh_page);

    if (s_refresh_page >= s_refresh_end_page)
    {
        s_refresh_pending = 0U;
    }
    else
    {
        s_refresh_page++;
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
    char line[20];

    OLED_ClearBuffer();

    (void)snprintf(line, sizeof(line), "%c TASK 1", (s_selected_task == 1U) ? '>' : ' ');
    OLED_DrawString(0U, 0U, line);

    (void)snprintf(line, sizeof(line), "%c TASK 2", (s_selected_task == 2U) ? '>' : ' ');
    OLED_DrawString(1U, 0U, line);

    (void)snprintf(line, sizeof(line), "%c TASK 3", (s_selected_task == 3U) ? '>' : ' ');
    OLED_DrawString(2U, 0U, line);

    (void)snprintf(line, sizeof(line), "%c TASK 4", (s_selected_task == 4U) ? '>' : ' ');
    OLED_DrawString(3U, 0U, line);

    OLED_RequestRefresh(3U);
}

static void OLED_RenderDashboard(void)
{
    char line[24];

    OLED_ClearBuffer();

    OLED_DrawString(0U, 24U, "--- RUNNING ---");

    (void)snprintf(line, sizeof(line), "Task: %u", s_running_task);
    OLED_DrawString(1U, 0U, line);

    (void)sprintf(line, "Yaw: %.2f", Car_Yaw);
    OLED_DrawString(2U, 0U, line);

    OLED_DrawString(3U, 0U, "EncL: 00000");

    OLED_DrawString(4U, 0U, "EncR: 00000");

    OLED_DrawString(5U, 0U, "Line: 001100");

    OLED_DrawString(6U, 0U, "Time: 00s");

    OLED_DrawString(7U, 0U, " ");

    OLED_RequestRefresh(7U);
}

static void OLED_UpdateDashboardYaw(void)
{
    char line[24];

    memset(&s_oled_buffer[(uint16_t)2U * OLED_WIDTH], 0x00, OLED_WIDTH);
    (void)sprintf(line, "Yaw: %.2f", Car_Yaw);
    OLED_DrawString(2U, 0U, line);
    OLED_RequestSinglePageRefresh(2U);
}

static void OLED_ServiceFeedback(uint32_t now)
{
    if (s_feedback_active != 0U && (int32_t)(now - s_feedback_off_tick) >= 0)
    {
        LED_OFF();
        BUZZER_OFF();
        s_feedback_active = 0U;
    }
}

static void OLED_EnterDashboard(void)
{
    s_running_task = s_selected_task;
    System_State = PAGE_DASHBOARD;

    LED_ON();
    BUZZER_ON();
    s_feedback_active = 1U;
    s_feedback_off_tick = (uint32_t)(HAL_GetTick() + OLED_FEEDBACK_MS);
    s_dashboard_refresh_tick = HAL_GetTick();

    OLED_Clear();
    OLED_RenderDashboard();
    OLED_RequestRefresh(7U);
}

void OLED_Menu_Init(void)
{
    System_State = PAGE_MENU;
    s_selected_task = 1U;
    s_running_task = 1U;
    s_menu_dirty = 1U;
    s_dashboard_dirty = 0U;
    s_refresh_pending = 0U;
    s_refresh_page = 0U;
    s_refresh_end_page = 3U;
    s_feedback_active = 0U;
    s_dashboard_refresh_tick = 0U;
    LED_OFF();
    BUZZER_OFF();

    OLED_InitHardware();
    OLED_Clear();
    OLED_RenderMenu();
    OLED_RequestRefresh(7U);
    s_menu_dirty = 0U;
}

void OLED_Menu_Process(void)
{
    ButtonEvent_t event;
    uint32_t now = HAL_GetTick();

    OLED_ServiceFeedback(now);

    while (Button_GetEvent(&event) != 0U)
    {
        if (System_State == PAGE_MENU)
        {
            if (event.event_type == BUTTON_EVENT_SHORT_PRESS && event.button_id == BUTTON_ID_MODE1)
            {
                s_selected_task = (s_selected_task >= 4U) ? 1U : (uint8_t)(s_selected_task + 1U);
                s_menu_dirty = 1U;
            }
            else if (event.event_type == BUTTON_EVENT_DOUBLE_CLICK && event.button_id == BUTTON_ID_MODE2)
            {
                OLED_EnterDashboard();
                break;
            }
        }
    }

    if (System_State == PAGE_MENU && s_menu_dirty != 0U)
    {
        OLED_RenderMenu();
        s_menu_dirty = 0U;
    }

    if (System_State == PAGE_DASHBOARD)
    {
        if (s_dashboard_dirty != 0U)
        {
            OLED_RenderDashboard();
            s_dashboard_dirty = 0U;
            s_dashboard_refresh_tick = now;
        }
        else if ((s_refresh_pending == 0U) && ((uint32_t)(now - s_dashboard_refresh_tick) >= 100U))
        {
            OLED_UpdateDashboardYaw();
            s_dashboard_refresh_tick = now;
        }
    }

    OLED_RefreshService();
}

uint8_t OLED_Menu_GetSelectedTask(void)
{
    return s_selected_task;
}

__weak void Task_Start(uint8_t task_num)
{
    (void)task_num;
}

