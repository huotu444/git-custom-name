#include "bluetooth.h"
#include "usart.h"

uint8_t g_bluetooth_rx_buf = 0;
volatile uint8_t g_bluetooth_cmd = 0;
volatile uint32_t g_rx_count = 0;
volatile uint32_t g_bluetooth_last_rx_tick = 0;

void Bluetooth_Init(void) {
    // 开启中断接收
    HAL_UART_Receive_IT(&huart1, &g_bluetooth_rx_buf, 1);
}

