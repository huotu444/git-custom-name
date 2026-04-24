#ifndef __TRACK_H
#define __TRACK_H

#include "main.h"

extern volatile uint8_t Sensor_Data[8];

void Track_Init(void);
float Track_GetError(void);
void Track_FollowLine(void);
uint8_t Track_IsLostLine(void);

#endif

