/* USER CODE BEGIN Header */
/**
    ******************************************************************************
    * @file    button.c
    * @brief   非阻塞按键扫描，支持短按和双击
    ******************************************************************************
    */
/* USER CODE END Header */
#include "button.h"

#define BUTTON_DEBOUNCE_MS           10U
#define BUTTON_DOUBLE_CLICK_MS      250U
#define BUTTON_SHORT_PRESS_MIN_MS     5U
#define BUTTON_EVENT_QUEUE_SIZE       8U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t raw_level;
    uint8_t stable_level;
    uint32_t raw_change_tick;
    uint32_t press_tick;
    uint32_t click_tick;
    uint8_t single_pending;
} ButtonRuntime_t;

static ButtonRuntime_t s_buttons[2] =
{
    {GPIOC, GPIO_PIN_14, 1U, 1U, 0U, 0U, 0U, 0U},
    {GPIOC, GPIO_PIN_15, 1U, 1U, 0U, 0U, 0U, 0U}
};

static ButtonEvent_t s_event_queue[BUTTON_EVENT_QUEUE_SIZE];
static uint8_t s_event_head = 0U;
static uint8_t s_event_tail = 0U;

static uint8_t Button_ReadRaw(const ButtonRuntime_t *button)
{
    return (HAL_GPIO_ReadPin(button->port, button->pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

static void Button_PushEvent(ButtonIdTypeDef button_id, ButtonEventTypeDef event_type)
{
    uint8_t next_head = (uint8_t)((s_event_head + 1U) % BUTTON_EVENT_QUEUE_SIZE);

    if (next_head == s_event_tail)
    {
        s_event_tail = (uint8_t)((s_event_tail + 1U) % BUTTON_EVENT_QUEUE_SIZE);
    }

    s_event_queue[s_event_head].button_id = button_id;
    s_event_queue[s_event_head].event_type = event_type;
    s_event_head = next_head;
}

void Button_Init(void)
{
    for (uint8_t index = 0U; index < 2U; index++)
    {
        s_buttons[index].raw_level = Button_ReadRaw(&s_buttons[index]);
        s_buttons[index].stable_level = s_buttons[index].raw_level;
        s_buttons[index].raw_change_tick = HAL_GetTick();
        s_buttons[index].press_tick = 0U;
        s_buttons[index].click_tick = 0U;
        s_buttons[index].single_pending = 0U;
    }

    s_event_head = 0U;
    s_event_tail = 0U;
}

void Button_Scan(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t index = 0U; index < 2U; index++)
    {
        ButtonRuntime_t *button = &s_buttons[index];
        uint8_t raw_level = Button_ReadRaw(button);

        if (raw_level != button->raw_level)
        {
            button->raw_level = raw_level;
            button->raw_change_tick = now;
        }

        if ((now - button->raw_change_tick) >= BUTTON_DEBOUNCE_MS && button->stable_level != button->raw_level)
        {
            button->stable_level = button->raw_level;

            if (button->stable_level == 1U)
            {
                button->press_tick = now;
            }
            else
            {
                uint32_t hold_time = now - button->press_tick;

                if (hold_time >= BUTTON_SHORT_PRESS_MIN_MS)
                {
                    if (button->single_pending != 0U && (now - button->click_tick) <= BUTTON_DOUBLE_CLICK_MS)
                    {
                        Button_PushEvent((ButtonIdTypeDef)index, BUTTON_EVENT_DOUBLE_CLICK);
                        button->single_pending = 0U;
                    }
                    else
                    {
                        Button_PushEvent((ButtonIdTypeDef)index, BUTTON_EVENT_SHORT_PRESS);
                        button->single_pending = 1U;
                        button->click_tick = now;
                    }
                }
                else
                {
                    button->single_pending = 0U;
                }
            }
        }

        if (button->single_pending != 0U && (now - button->click_tick) > BUTTON_DOUBLE_CLICK_MS)
        {
            button->single_pending = 0U;
        }
    }
}

uint8_t Button_GetEvent(ButtonEvent_t *event)
{
    if (event == NULL)
    {
        return 0U;
    }

    if (s_event_tail == s_event_head)
    {
        return 0U;
    }

    *event = s_event_queue[s_event_tail];
    s_event_tail = (uint8_t)((s_event_tail + 1U) % BUTTON_EVENT_QUEUE_SIZE);
    return 1U;
}

