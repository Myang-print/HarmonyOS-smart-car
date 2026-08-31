#include "stm32f10x.h"
#include "sys.h"
#include "control_system.h"
#include "colorful_led.h"
#include "autonomous_protocol.h"
#include "autonomous_behavior.h"
#include "soft_uart_rx.h"

#define CONTROL_LOOP_MS ((u16)20)
#define LED_UPDATE_MS ((u16)100)

static void ApplyLedEffect(AutonomousMotion motion, u8 link_alive) {
  if (!link_alive) {
    LED_State_LinkFault_Step();
    return;
  }

  switch (motion) {
  case AUTONOMOUS_MOTION_FORWARD:
  case AUTONOMOUS_MOTION_PROBE:
    LED_State_Forward_Step();
    break;
  case AUTONOMOUS_MOTION_REVERSE:
    LED_State_Reverse_Step();
    break;
  case AUTONOMOUS_MOTION_LEFT:
    LED_State_Left_Step();
    break;
  case AUTONOMOUS_MOTION_RIGHT:
    LED_State_Right_Step();
    break;
  case AUTONOMOUS_MOTION_STOP:
  case AUTONOMOUS_MOTION_BRAKE:
    LED_State_Stop_Step();
    break;
  default:
    LED_All_Off_Step();
    break;
  }
}

static void ApplyMotion(AutonomousMotion motion) {
  switch (motion) {
  case AUTONOMOUS_MOTION_BRAKE:
    Control_SetTargetWheels(0, 0);
    break;
  case AUTONOMOUS_MOTION_FORWARD:
    Control_SetTargetWheels(CAR_FORWARD_SPEED, CAR_FORWARD_SPEED);
    break;
  case AUTONOMOUS_MOTION_REVERSE:
    Control_SetTargetWheels(-CAR_REVERSE_SPEED, -CAR_REVERSE_SPEED);
    break;
  case AUTONOMOUS_MOTION_LEFT:
    Control_SetTargetWheels(-CAR_TURN_SPEED, CAR_TURN_SPEED);
    break;
  case AUTONOMOUS_MOTION_RIGHT:
    Control_SetTargetWheels(CAR_TURN_SPEED, -CAR_TURN_SPEED);
    break;
  case AUTONOMOUS_MOTION_PROBE:
    Control_SetTargetWheels(CAR_PROBE_SPEED, CAR_PROBE_SPEED);
    break;
  default:
    Control_Stop();
    break;
  }
}

/**************************************************************************
通信连接：Hi3861 GPIO6(UART1_TX)->STM32 PB8(软件UART_RX)，2400-8-N-1
帧格式：A5 5A SEQ CMD XOR；连续收到完整校验帧才建立通信连接
安全策略：1000ms未收到有效帧立即停车，悬空PB8由内部上拉保持UART空闲态
**************************************************************************/
int main(void) {
  AutonomousProtocol protocol;
  AutonomousBehavior behavior;
  u8 received_byte;
  u8 safe_command;
  u8 link_alive;
  AutonomousMotion motion;
  AutonomousMotion applied_motion = AUTONOMOUS_MOTION_STOP;
  u32 now_ms = 0;
  u32 rx_byte_count = 0;
  u32 last_rx_byte_count = 0;
  u16 led_elapsed_ms = LED_UPDATE_MS;
  u16 rx_quiet_ms = LED_UPDATE_MS;

  Stm32_Clock_Init(9);
  MY_NVIC_PriorityGroupConfig(2);
  JTAG_Set(JTAG_SWD_DISABLE);
  JTAG_Set(SWD_ENABLE);

  Control_System_Init();
  SoftUartRx_Init();
  colorful_led_Init();
  AutonomousProtocol_Init(&protocol);
  AutonomousBehavior_Init(&behavior, now_ms);
  Control_Stop();
  LED_All_Off_Step();

  while (1) {
    while (SoftUartRx_ReadByte(&received_byte))
      AutonomousProtocol_PushByte(&protocol, received_byte, now_ms);

    rx_byte_count = SoftUartRx_GetByteCount();
    if (rx_byte_count != last_rx_byte_count) {
      last_rx_byte_count = rx_byte_count;
      rx_quiet_ms = 0U;
    }

    link_alive = AutonomousProtocol_HasLink(&protocol, now_ms);
    safe_command = AutonomousProtocol_GetSafeCommand(&protocol, now_ms);
    motion = AutonomousBehavior_Update(&behavior, safe_command, now_ms);
    if (motion != applied_motion) {
      ApplyMotion(motion);
      applied_motion = motion;
    }

    Control_Update();
    /* Keep WS2812 bit-banging outside active serial sampling windows. */
    if (led_elapsed_ms >= LED_UPDATE_MS &&
        rx_quiet_ms >= CONTROL_LOOP_MS && !SoftUartRx_IsReceiving()) {
      ApplyLedEffect(motion, link_alive);
      led_elapsed_ms = 0;
    }

    delay_ms(CONTROL_LOOP_MS);
    now_ms += CONTROL_LOOP_MS;
    led_elapsed_ms += CONTROL_LOOP_MS;
    if (rx_quiet_ms <= (u16)(0xFFFFU - CONTROL_LOOP_MS))
      rx_quiet_ms += CONTROL_LOOP_MS;
  }
}
