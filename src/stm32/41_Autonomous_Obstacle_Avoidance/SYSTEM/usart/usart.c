#include "usart.h"

/* 支持printf重定向，避免使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE {
  int handle;
};

FILE __stdout;

void _sys_exit(int x) { (void)x; }

int fputc(int ch, FILE *stream) {
  (void)stream;
  while ((USART1->SR & USART_FLAG_TC) == 0)
    ;
  USART1->DR = (u8)ch;
  return ch;
}

#if EN_USART1_RX

#define UART_RX_BUFFER_SIZE 32U

static volatile u8 received_buffer[UART_RX_BUFFER_SIZE];
static volatile u8 received_head;
static volatile u8 received_tail;
static volatile u32 received_count;
static volatile u32 error_count;
static volatile u32 overflow_count;

/**************************************************************************
函数功能：初始化USART1
入口参数：bound，串口波特率
返回值  ：无
硬件连接：PA9-TX，PA10-RX
**************************************************************************/
void uart_init(u32 bound) {
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA,
                         ENABLE);

  gpio.GPIO_Pin = GPIO_Pin_9;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_10;
  gpio.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &gpio);

  nvic.NVIC_IRQChannel = USART1_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0;
  nvic.NVIC_IRQChannelSubPriority = 0;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  USART_StructInit(&usart);
  usart.USART_BaudRate = bound;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &usart);

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void) {
  u8 data;
  u8 next_head;
  u16 status;

  status = USART1->SR;
  if ((status & (USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE |
                 USART_FLAG_ORE)) != 0) {
    error_count++;
    data = (u8)USART1->DR;
    if ((status & USART_FLAG_RXNE) == 0)
      return;
  } else if ((status & USART_FLAG_RXNE) != 0) {
    data = (u8)USART1->DR;
  } else {
    return;
  }
  received_count++;
  next_head = (u8)((received_head + 1U) % UART_RX_BUFFER_SIZE);
  if (next_head != received_tail) {
    received_buffer[received_head] = data;
    received_head = next_head;
  } else
    overflow_count++;
}

u32 UART_GetRxCount(void) { return received_count; }

u32 UART_GetErrorCount(void) { return error_count; }

u32 UART_GetOverflowCount(void) { return overflow_count; }

/**************************************************************************
函数功能：非阻塞读取一个USART1指令字节
入口参数：data，接收字节的保存地址
返回值  ：1表示读取成功，0表示当前无数据或参数无效
**************************************************************************/
u8 UART_TryReadByte(u8 *data) {
  if (data == 0)
    return 0;

  if (received_tail == received_head)
    return 0;

  *data = received_buffer[received_tail];
  received_tail = (u8)((received_tail + 1U) % UART_RX_BUFFER_SIZE);
  return 1;
}

void UART_WriteBytes(const u8 *data, u8 length) {
  u8 i;

  if (data == 0)
    return;
  for (i = 0; i < length; i++) {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
      ;
    USART_SendData(USART1, data[i]);
  }
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    ;
}

#endif
