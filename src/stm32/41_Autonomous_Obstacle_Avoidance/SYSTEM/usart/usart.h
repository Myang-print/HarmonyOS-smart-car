#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "sys.h"

#define EN_USART1_RX 1

void uart_init(u32 bound);
/* 非阻塞读取一个串口指令字节：有数据返回1，无数据返回0 */
u8 UART_TryReadByte(u8 *data);
void UART_WriteBytes(const u8 *data, u8 length);
u32 UART_GetRxCount(void);
u32 UART_GetErrorCount(void);
u32 UART_GetOverflowCount(void);

#endif
