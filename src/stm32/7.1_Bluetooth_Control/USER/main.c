#include "stm32f10x.h"
#include "sys.h"
#include "control_system.h"
#include "colorful_led.h"
#include "delay.h"
#include "usart.h"

/**************************************************************************
函数功能：初始化小车并持续执行 Hi3861 下发的左右轮速度指令
入口参数：无
返回值  ：程序持续循环，不返回
协议    ：FC 左方向 左速度 右方向 右速度 FD，115200-8-N-1
**************************************************************************/
int main(void)
{
    UartMotorCommand command;
    u8 command_received;
    u8 receive_events;

    Stm32_Clock_Init(9);              /* 外部8MHz，系统时钟72MHz */
    MY_NVIC_PriorityGroupConfig(2);
    uart_init(115200);                /* PA10接收Hi3861 GPIO11/UART2_TX */
    JTAG_Set(JTAG_SWD_DISABLE);
    JTAG_Set(SWD_ENABLE);             /* 保留ST-Link/SWD烧录调试 */

    Control_System_Init();
    colorful_led_Init();
    Control_Stop();
    LED_All_Off_Step();

    printf("REMOTE,READY,USART1,PA10,115200,FC-FD\r\n");

    /* 上电灯光自检期间电机始终停止。 */
    LED_Diagnostic_Boot_Step();
    delay_ms(300);
    LED_State_Forward_Step();
    delay_ms(300);
    LED_State_Reverse_Step();
    delay_ms(300);
    LED_State_Left_Step();
    delay_ms(300);
    LED_State_Right_Step();
    delay_ms(300);
    Control_Stop();

    while (1)
    {
        command_received = UART_TryReadMotorCommand(&command);
        receive_events = UART_TakeRxEvents();

        if (command_received)
        {
            Control_SetRemoteTarget(command.left_speed_x100,
                                    command.right_speed_x100);
            printf("REMOTE,CMD,L=%d,R=%d\r\n",
                   command.left_speed_x100, command.right_speed_x100);
        }
        else if (receive_events & UART_RX_EVENT_INVALID_FRAME)
        {
            Control_ReportProtocolError();
            printf("REMOTE,INVALID_FRAME\r\n");
        }
        else if (receive_events & UART_RX_EVENT_ACTIVITY)
        {
            Control_ReportUartActivity();
        }

        Control_RemoteStep();
        delay_ms(CAR_CONTROL_PERIOD_MS);
    }
}
