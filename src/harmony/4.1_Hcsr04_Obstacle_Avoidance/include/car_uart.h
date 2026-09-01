#ifndef CAR_UART_H
#define CAR_UART_H

#include <stdbool.h>
#include <stdint.h>

bool CarUart_Init(void);
bool CarUart_SendCommand(uint8_t command);

#endif
