#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "sys.h"

#define EN_USART1_RX 1
#define UART_MOTOR_MAX_SPEED_X100 ((u8)250)
#define UART_RX_EVENT_ACTIVITY      ((u8)0x01)
#define UART_RX_EVENT_VALID_FRAME   ((u8)0x02)
#define UART_RX_EVENT_INVALID_FRAME ((u8)0x04)

typedef struct {
  s16 left_speed_x100;
  s16 right_speed_x100;
} UartMotorCommand;

void uart_init(u32 bound);
/* 非阻塞读取协议帧解码后的左右轮有符号速度。 */
u8 UART_TryReadMotorCommand(UartMotorCommand *command);
/* 读取并清除串口活动、合法帧和非法帧事件。 */
u8 UART_TakeRxEvents(void);

#endif
