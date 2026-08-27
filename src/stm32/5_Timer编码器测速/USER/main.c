#include "stm32f10x.h"
#include "sys.h"
#include "control_system.h"

#define CAR_ACTION_TIME_MS ((u16)2000)
#define CAR_STOP_TIME_MS ((u16)800)

/**************************************************************************
函数功能：按指定速度驱动左右轮一段时间，随后停车
入口参数：left_speed左轮速度，right_speed右轮速度，run_time运行时间
返回值  ：无
**************************************************************************/
static void RunTimedAction(s16 left_speed, s16 right_speed, u16 run_time) {
  Control_SetWheels(left_speed, right_speed);
  delay_ms(run_time);
  Control_Stop();
  delay_ms(CAR_STOP_TIME_MS);
}

/**************************************************************************
函数功能：系统复位后按时间依次测试单轮、双轮和正反转
入口参数：无
返回值  ：程序不退出
**************************************************************************/
int main(void) {
  /* 系统基础初始化 */
  Stm32_Clock_Init(9);
  MY_NVIC_PriorityGroupConfig(2);
  JTAG_Set(JTAG_SWD_DISABLE);
  JTAG_Set(SWD_ENABLE);

  Control_System_Init(); // 初始化完成后默认停车
  delay_ms(3000);        // 留出放置小车和松开复位键的时间

  /* 按固定时间循环执行全部基础动作 */
  while (1) {
    RunTimedAction(CAR_TEST_SPEED, 0, CAR_ACTION_TIME_MS);  // 左轮正转
    RunTimedAction(-CAR_TEST_SPEED, 0, CAR_ACTION_TIME_MS); // 左轮反转
    RunTimedAction(0, CAR_TEST_SPEED, CAR_ACTION_TIME_MS);  // 右轮正转
    RunTimedAction(0, -CAR_TEST_SPEED, CAR_ACTION_TIME_MS); // 右轮反转

    RunTimedAction(CAR_TEST_SPEED, CAR_TEST_SPEED,
                   CAR_ACTION_TIME_MS); // 双轮正转
    RunTimedAction(-CAR_TEST_SPEED, -CAR_TEST_SPEED,
                   CAR_ACTION_TIME_MS); // 双轮反转

    RunTimedAction(-CAR_TEST_SPEED, CAR_TEST_SPEED,
                   CAR_ACTION_TIME_MS); // 原地左转
    RunTimedAction(CAR_TEST_SPEED, -CAR_TEST_SPEED,
                   CAR_ACTION_TIME_MS); // 原地右转
  }
}
