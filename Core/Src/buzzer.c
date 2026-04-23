#include "buzzer.h"

/**
  * @brief 蜂鸣器初始化 (PA15)
  * @note  PA15 默认是 JTAG 引脚，必须通过 AFIO 重映射禁用 JTAG 才能用作普通 GPIO
  */
void Buzzer_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 开启时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE(); // 重要：修改 JTAG 引脚必须开启 AFIO 时钟

    // 2. 禁用 JTAG，保留 SWD (确保 ST-Link 还能烧录)
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    // 3. 配置 PA15 为输出
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    // Active-Low：初始必须拉高保持静音
    BUZZER_OFF();
}

/**
  * @brief 简单警报
  */
void Buzzer_Alarm(uint16_t duration_ms) {
    BUZZER_ON();
    HAL_Delay(duration_ms);
    BUZZER_OFF();
}
