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

    Stm32_Clock_Init(9);              // 外部 8MHz 晶振，系统时钟 72MHz
    MY_NVIC_PriorityGroupConfig(2);   // 设置中断优先级分组
    uart_init(115200);                // PA10 接收 Hi3861 UART2_TX
    JTAG_Set(JTAG_SWD_DISABLE);       // 关闭 JTAG，释放相关 IO
    JTAG_Set(SWD_ENABLE);             // 保留 SWD 下载调试接口

    Control_System_Init();            // 初始化 TIM4 PWM、TIM2/TIM3 编码器
    colorful_led_Init();              // 初始化前后两组 WS2812 灯带
    Control_Stop();
    LED_All_Off_Step();

    printf("REMOTE,READY,FC-DL-SL-DR-SR-FD\r\n");

    /* 上电灯光自检：白、绿、红、蓝、橙；全程保持电机停止。 */
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
