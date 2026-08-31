#ifndef __SOFT_UART_RX_H
#define __SOFT_UART_RX_H

/* Hi3861 UART1 TX (GPIO6) is connected to STM32 PB8. */
#define SOFT_UART_RX_BAUD 2400U

void SoftUartRx_Init(void);
unsigned char SoftUartRx_ReadByte(unsigned char *byte);
unsigned char SoftUartRx_IsReceiving(void);
unsigned long SoftUartRx_GetByteCount(void);
unsigned long SoftUartRx_GetOverflowCount(void);

#endif
