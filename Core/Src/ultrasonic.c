#include "ultrasonic.h"
#include "tim.h"

/**
  * @brief 微秒级延时函数
  * @param us 延时微秒数
  * @note 需确保 TIM4 的 PSC 设置为 71 (72MHz 频率下)
  */
static void Delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim4, 0); 
    while (__HAL_TIM_GET_COUNTER(&htim4) < us);
}

/**
  * @brief 超声波测距核心函数
  * @return 距离值，单位 cm
  */
float Ultrasonic_GetDistance(void) {
    uint32_t start_time = 0, end_time = 0;
    
    // 1. Trig 发送 15us 高电平触发信号 (更可靠)
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    Delay_us(15);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    
    // 2. 等待 Echo 变高 (超时 50ms)
    uint32_t timeout = 50000;
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET && timeout--);
    if (timeout == 0) return 999.0f; 
    
    start_time = __HAL_TIM_GET_COUNTER(&htim4);
    
    // 3. 等待 Echo 变低 (超时 50ms)
    timeout = 50000;
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET && timeout--);
    if (timeout == 0) return 999.0f;
    
    end_time = __HAL_TIM_GET_COUNTER(&htim4);
    
    // 4. 计算距离 (声速 340m/s -> 0.034cm/us -> 往返除以 2 为 0.017)
    float distance = (float)(end_time - start_time) * 0.017f;
    
    // 过滤极端异常值
    if (distance > 400.0f) return 400.0f;
    if (distance < 2.0f) return 0.0f;
    
    return distance;
}
