#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "main.h"

// 全局蓝牙控制命令寄存
extern uint8_t g_bluetooth_rx_buf;
extern volatile uint8_t g_bluetooth_cmd;
extern volatile uint32_t g_rx_count;
extern volatile uint32_t g_bluetooth_last_rx_tick;

void Bluetooth_Init(void);

#endif

