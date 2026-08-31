#include "soft_uart_rx.h"

#include "stm32f10x.h"

#define SOFT_UART_RX_PIN GPIO_Pin_8
#define SOFT_UART_RX_EXTI_LINE EXTI_Line8
#define SOFT_UART_RX_BUFFER_SIZE 32U
#define SOFT_UART_RX_BUFFER_MASK (SOFT_UART_RX_BUFFER_SIZE - 1U)

/* TIM3 runs at 1 MHz. 2400 baud is 416.67 us per bit. */
#define SOFT_UART_BIT_TICKS 417U
#define SOFT_UART_FIRST_SAMPLE_TICKS 625U

static volatile unsigned char rx_buffer[SOFT_UART_RX_BUFFER_SIZE];
static volatile unsigned char rx_head;
static volatile unsigned char rx_tail;
static volatile unsigned char rx_byte;
static volatile unsigned char rx_bit_index;
static volatile unsigned char rx_receiving;
static volatile unsigned long rx_byte_count;
static volatile unsigned long rx_overflow_count;

static void PushByteFromInterrupt(unsigned char byte) {
  unsigned char next = (unsigned char)((rx_head + 1U) &
                                       SOFT_UART_RX_BUFFER_MASK);

  if (next == rx_tail) {
    rx_overflow_count++;
    return;
  }
  rx_buffer[rx_head] = byte;
  rx_head = next;
  rx_byte_count++;
}

static void RearmStartInterrupt(void) {
  TIM_Cmd(TIM3, DISABLE);
  TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
  TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
  EXTI_ClearITPendingBit(SOFT_UART_RX_EXTI_LINE);
  EXTI->IMR |= SOFT_UART_RX_EXTI_LINE;
  rx_receiving = 0U;
}

void SoftUartRx_Init(void) {
  GPIO_InitTypeDef gpio;
  EXTI_InitTypeDef exti;
  TIM_TimeBaseInitTypeDef timer;
  NVIC_InitTypeDef nvic;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
                         ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  gpio.GPIO_Pin = SOFT_UART_RX_PIN;
  gpio.GPIO_Mode = GPIO_Mode_IPU;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &gpio);

  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
  exti.EXTI_Line = SOFT_UART_RX_EXTI_LINE;
  exti.EXTI_Mode = EXTI_Mode_Interrupt;
  exti.EXTI_Trigger = EXTI_Trigger_Falling;
  exti.EXTI_LineCmd = ENABLE;
  EXTI_Init(&exti);
  EXTI_ClearITPendingBit(SOFT_UART_RX_EXTI_LINE);

  timer.TIM_Prescaler = 71U;
  timer.TIM_CounterMode = TIM_CounterMode_Up;
  timer.TIM_Period = SOFT_UART_BIT_TICKS - 1U;
  timer.TIM_ClockDivision = TIM_CKD_DIV1;
  timer.TIM_RepetitionCounter = 0U;
  TIM_TimeBaseInit(TIM3, &timer);
  TIM_Cmd(TIM3, DISABLE);
  TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
  TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

  nvic.NVIC_IRQChannel = TIM3_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0U;
  nvic.NVIC_IRQChannelSubPriority = 0U;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0U;
  nvic.NVIC_IRQChannelSubPriority = 1U;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  rx_head = 0U;
  rx_tail = 0U;
  rx_byte = 0U;
  rx_bit_index = 0U;
  rx_receiving = 0U;
  rx_byte_count = 0UL;
  rx_overflow_count = 0UL;
}

unsigned char SoftUartRx_ReadByte(unsigned char *byte) {
  if (byte == 0 || rx_tail == rx_head)
    return 0U;

  *byte = rx_buffer[rx_tail];
  rx_tail = (unsigned char)((rx_tail + 1U) & SOFT_UART_RX_BUFFER_MASK);
  return 1U;
}

unsigned char SoftUartRx_IsReceiving(void) {
  return rx_receiving;
}

unsigned long SoftUartRx_GetByteCount(void) {
  return rx_byte_count;
}

unsigned long SoftUartRx_GetOverflowCount(void) {
  return rx_overflow_count;
}

void EXTI9_5_IRQHandler(void) {
  if (EXTI_GetITStatus(SOFT_UART_RX_EXTI_LINE) == RESET)
    return;

  EXTI_ClearITPendingBit(SOFT_UART_RX_EXTI_LINE);
  if (GPIO_ReadInputDataBit(GPIOB, SOFT_UART_RX_PIN) != Bit_RESET)
    return;

  EXTI->IMR &= ~SOFT_UART_RX_EXTI_LINE;
  rx_byte = 0U;
  rx_bit_index = 0U;
  rx_receiving = 1U;

  TIM_SetAutoreload(TIM3, SOFT_UART_FIRST_SAMPLE_TICKS - 1U);
  TIM_SetCounter(TIM3, 0U);
  TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM3, ENABLE);
}

void TIM3_IRQHandler(void) {
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) == RESET)
    return;

  TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
  if (!rx_receiving) {
    RearmStartInterrupt();
    return;
  }

  if (rx_bit_index < 8U) {
    if (GPIO_ReadInputDataBit(GPIOB, SOFT_UART_RX_PIN) != Bit_RESET)
      rx_byte |= (unsigned char)(1U << rx_bit_index);
    rx_bit_index++;
    TIM_SetAutoreload(TIM3, SOFT_UART_BIT_TICKS - 1U);
    return;
  }

  /* A valid 8N1 byte must have a high stop bit. */
  if (GPIO_ReadInputDataBit(GPIOB, SOFT_UART_RX_PIN) != Bit_RESET)
    PushByteFromInterrupt(rx_byte);
  RearmStartInterrupt();
}
