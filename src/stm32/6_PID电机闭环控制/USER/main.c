#include "stm32f10x.h"
#include "sys.h"
#include "control_system.h"

/**************************************************************************
函数功能：执行一次“直行 + 后退”组合动作
入口参数：无
返回值  ：无
说明    ：两个动作均采用双轮速度 PI 和左右轮同步纠偏
**************************************************************************/
static void RunForwardReversePair(void)
{
    printf("ACTION: forward\r\n");
    Control_RunTimedPID(1, 1, CAR_STRAIGHT_TIME_MS,
                        LED_Colorful_Ring_Step);

    printf("ACTION: reverse\r\n");
    Control_RunTimedPID(-1, -1, CAR_REVERSE_TIME_MS,
                        LED_Red_Breathing_Step);
}

/**************************************************************************
函数功能：依次演示单轮转向、双轮原地转向及转向后的直行和后退
入口参数：无
返回值  ：程序持续循环，不返回
**************************************************************************/
int main(void)
{
    Stm32_Clock_Init(9);              // 外部 8MHz 晶振，系统时钟 72MHz
    MY_NVIC_PriorityGroupConfig(2);   // 设置中断优先级分组
    uart_init(115200);                // 串口用于输出动作状态和超时提示
    JTAG_Set(JTAG_SWD_DISABLE);       // 关闭 JTAG，释放相关 IO
    JTAG_Set(SWD_ENABLE);             // 保留 SWD 下载调试接口

    Control_System_Init();            // 初始化 TIM4 PWM、TIM2/TIM3 编码器
    colorful_led_Init();              // 初始化前后两组 WS2812 灯带
    Control_Stop();
    LED_All_Off_Step();

    printf("PID car demo ready\r\n");
    delay_ms(3000);                   // 留出放置小车和松开复位键的时间

    while (1)
    {
        /* 单轮右转 90 度：左轮前进，红色顺时针跑马灯。 */
        printf("ACTION: pivot right 90\r\n");
        Control_RunEncoderPID(1, 0, CAR_PIVOT_90_COUNTS,
                              CAR_TURN_TIMEOUT_MS,
                              LED_Red_Clockwise_Step);
        RunForwardReversePair();

        /* 单轮左转 90 度：右轮前进，蓝色逆时针跑马灯。 */
        printf("ACTION: pivot left 90\r\n");
        Control_RunEncoderPID(0, 1, CAR_PIVOT_90_COUNTS,
                              CAR_TURN_TIMEOUT_MS,
                              LED_Blue_CounterClockwise_Step);
        RunForwardReversePair();

        /* 双轮原地右转 90 度：左前右后，红色顺时针渐变跑马灯。 */
        printf("ACTION: spin right 90\r\n");
        Control_RunEncoderPID(1, -1, CAR_SPIN_90_COUNTS,
                              CAR_TURN_TIMEOUT_MS,
                              LED_Red_Clockwise_Gradient_Step);
        RunForwardReversePair();

        /* 双轮原地左转 90 度：左后右前，蓝色逆时针渐变跑马灯。 */
        printf("ACTION: spin left 90\r\n");
        Control_RunEncoderPID(-1, 1, CAR_SPIN_90_COUNTS,
                              CAR_TURN_TIMEOUT_MS,
                              LED_Blue_CounterClockwise_Gradient_Step);
        RunForwardReversePair();
    }
}
