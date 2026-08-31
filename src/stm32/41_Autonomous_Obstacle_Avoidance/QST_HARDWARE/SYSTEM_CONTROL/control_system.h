#ifndef __CONTROL_SYSTEM_H
#define __CONTROL_SYSTEM_H

#include "stm32f10x.h"

/* TIM4 PWM参数：72MHz / (9 + 1) / (7199 + 1) = 1kHz */
#define CAR_PWM_PERIOD ((u16)7199)
#define CAR_PWM_PRESCALER ((u16)9)
#define CAR_FORWARD_SPEED ((s16)4200)
#define CAR_REVERSE_SPEED ((s16)3200)
#define CAR_TURN_SPEED ((s16)3600)
#define CAR_PROBE_SPEED ((s16)2800)
#define CAR_SPEED_RAMP_STEP ((s16)250)
#define CAR_TEST_SPEED ((s16)6000)

/* 电机控制接口 */
void Control_System_Init(void);
void Control_SetWheels(s16 left_speed, s16 right_speed);
void Control_SetTargetWheels(s16 left_speed, s16 right_speed);
void Control_Stop(void);
void Control_Update(void);
u8 Control_ExecuteCommand(u8 command);
void Control_RunSelfTest(u16 *left_edges, u16 *right_edges);

#endif
