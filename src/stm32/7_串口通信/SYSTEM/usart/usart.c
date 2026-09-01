#include "usart.h"

/* 支持 printf 重定向，避免使用半主机模式。 */
#pragma import(__use_no_semihosting)

#define UART_MOTOR_FRAME_HEADER ((u8)0xFC)
#define UART_MOTOR_FRAME_TAIL   ((u8)0xFD)
#define UART_MOTOR_FRAME_SIZE   ((u8)6)

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

static u8 receive_frame[UART_MOTOR_FRAME_SIZE];
static volatile u8 receive_index;
static volatile s16 pending_left_speed_x100;
static volatile s16 pending_right_speed_x100;
static volatile u8 motor_command_ready;
static volatile u8 receive_events;

/**************************************************************************
函数功能：解析 Hi3861 发来的一个字节
入口参数：data，USART1 接收到的字节
返回值  ：无
协议    ：FC 左方向 左速度 右方向 右速度 FD
说明    ：方向 0 为正转、1 为反转；速度范围 0~250，单位为 0.01rad/s
**************************************************************************/
static void UART_ParseMotorByte(u8 data) {
  s16 left_speed;
  s16 right_speed;

  /* 速度字段最大为 250，因此 0xFC 可在任意位置安全地用于重新同步。 */
  if (data == UART_MOTOR_FRAME_HEADER) {
    receive_frame[0] = data;
    receive_index = 1;
    return;
  }

  if (receive_index == 0)
    return;

  receive_frame[receive_index] = data;
  receive_index++;
  if (receive_index < UART_MOTOR_FRAME_SIZE)
    return;

  receive_index = 0;
  if (receive_frame[5] != UART_MOTOR_FRAME_TAIL
      || receive_frame[1] > 1 || receive_frame[3] > 1
      || receive_frame[2] > UART_MOTOR_MAX_SPEED_X100
      || receive_frame[4] > UART_MOTOR_MAX_SPEED_X100) {
    receive_events |= UART_RX_EVENT_INVALID_FRAME;
    return;
  }

  left_speed = (s16)receive_frame[2];
  right_speed = (s16)receive_frame[4];
  if (receive_frame[1] == 1)
    left_speed = (s16)-left_speed;
  if (receive_frame[3] == 1)
    right_speed = (s16)-right_speed;

  /* 仅发布完整且合法的帧；新帧覆盖尚未消费的旧帧。 */
  pending_left_speed_x100 = left_speed;
  pending_right_speed_x100 = right_speed;
  motor_command_ready = 1;
  receive_events |= UART_RX_EVENT_VALID_FRAME;
}

#endif

/**************************************************************************
函数功能：初始化 USART1
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
  gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &gpio);

  nvic.NVIC_IRQChannel = USART1_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 3;
  nvic.NVIC_IRQChannelSubPriority = 3;
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

#if EN_USART1_RX
  receive_index = 0;
  motor_command_ready = 0;
  receive_events = 0;
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif
  USART_Cmd(USART1, ENABLE);
}

#if EN_USART1_RX

void USART1_IRQHandler(void) {
  u8 data;

  if (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET)
    return;

  data = (u8)USART_ReceiveData(USART1);
  receive_events |= UART_RX_EVENT_ACTIVITY;
  UART_ParseMotorByte(data);
}

/**************************************************************************
函数功能：非阻塞读取一条完整的左右轮控制指令
入口参数：command，接收已解码有符号速度的结构体地址
返回值  ：1 表示读取成功，0 表示当前无完整帧或参数无效
**************************************************************************/
u8 UART_TryReadMotorCommand(UartMotorCommand *command) {
  if (command == 0)
    return 0;

  /* 临界区只复制两个半字，避免 ISR 发布新帧时读到混合数据。 */
  USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  if (motor_command_ready == 0) {
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    return 0;
  }

  command->left_speed_x100 = pending_left_speed_x100;
  command->right_speed_x100 = pending_right_speed_x100;
  motor_command_ready = 0;
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  return 1;
}

u8 UART_TakeRxEvents(void) {
  u8 events;

  USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  events = receive_events;
  receive_events = 0;
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  return events;
}

#endif
