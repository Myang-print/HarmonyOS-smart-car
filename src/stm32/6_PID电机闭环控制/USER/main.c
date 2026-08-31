#include "stm32f10x.h"
#include "sys.h"
#include "control_system.h"
#include "colorful_led.h"
#include "delay.h"

/**************************************************************************
函数功能：执行一次包含四种运动状态的闭合周期路径
入口参数：无
返回值  ：1表示路径完成，0表示某次90度转向超时
说明    ：理论路径会回到起点和原朝向，便于后续接入避障状态机
**************************************************************************/
static u8 RunClosedPathCycle(void)
{
    printf("PATH,1,FORWARD\r\n");
    Control_MoveForward(CAR_PATH_SEGMENT_TIME_MS);

    printf("PATH,2,RIGHT_90\r\n");
    if (Control_TurnRight90() == 0)
        return 0;

    printf("PATH,3,FORWARD\r\n");
    Control_MoveForward(CAR_PATH_SEGMENT_TIME_MS);

    printf("PATH,4,LEFT_90\r\n");
    if (Control_TurnLeft90() == 0)
        return 0;

    printf("PATH,5,REVERSE\r\n");
    Control_MoveReverse(CAR_PATH_SEGMENT_TIME_MS);

    printf("PATH,6,RIGHT_90\r\n");
    if (Control_TurnRight90() == 0)
        return 0;

    printf("PATH,7,REVERSE\r\n");
    Control_MoveReverse(CAR_PATH_SEGMENT_TIME_MS);

    printf("PATH,8,LEFT_90\r\n");
    if (Control_TurnLeft90() == 0)
        return 0;

    printf("PATH,CYCLE_COMPLETE\r\n");
    return 1;
}

/**************************************************************************
函数功能：初始化小车并周期执行闭合路径
入口参数：无
返回值  ：程序持续循环，不返回
**************************************************************************/
int main(void)
{
    Stm32_Clock_Init(9);              // 外部 8MHz 晶振，系统时钟 72MHz
    MY_NVIC_PriorityGroupConfig(2);   // 设置中断优先级分组
    uart_init(115200);                // 串口输出当前运动阶段
    JTAG_Set(JTAG_SWD_DISABLE);       // 关闭 JTAG，释放相关 IO
    JTAG_Set(SWD_ENABLE);             // 保留 SWD 下载调试接口

    Control_System_Init();            // 初始化 TIM4 PWM、TIM2/TIM3 编码器
    colorful_led_Init();              // 初始化前后两组 WS2812 灯带
    Control_Stop();
    LED_All_Off_Step();

    printf("PID state-light path ready\r\n");
    printf("LIGHT,F=BREATH,B=RED_BREATH,L=BLUE_CCW,R=RED_CW\r\n");
    delay_ms(3000);                   // 留出放置小车和松开复位键的时间

    while (1)
    {
        if (RunClosedPathCycle() == 0)
        {
            printf("PATH,ABORTED,RETRY_AFTER_3S\r\n");
            Control_Stop();
            LED_All_Off_Step();
            delay_ms(3000);
        }
    }
}
