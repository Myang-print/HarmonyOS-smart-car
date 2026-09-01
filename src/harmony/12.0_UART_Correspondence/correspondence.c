/*
 * ================================================================
 *              HI3861_MIXED_PATH_V5_ALL_ACTIONS
 *
 *                         +---------+
 *                         |         |
 *                         |  (^v^)  |  MIXED V5
 *                         |         |
 *                         +---------+
 *
 *                    .------------------.
 *               ____/   HI3861 -> STM32  \____
 *              |    _                  _      | --->
 *              '---O------------------O-------'
 *
 * V5正确固件行为：
 *   1. 上电后白灯停止3秒；
 *   2. 每个动作后白灯停止1秒，完整顺序如下：
 *      绿灯前进2秒 -> 蓝灯左转0.75秒 -> 绿灯前进2秒 -> 橙灯右转0.75秒
 *      -> 红灯后退2秒 -> 蓝灯左转0.75秒 -> 红灯后退2秒 -> 橙灯右转0.75秒
 *   3. 前两条边正向行驶，后两条边倒车返回；理论上回到起点和初始朝向。
 *   4. 一轮结束后白灯停止3秒，再开始下一轮。
 *
 * 固件版本判断：
 *   实际编译目录必须能够搜索到：HI3861_MIXED_PATH_V5_ALL_ACTIONS
 *   如果只有蓝灯和绿灯交替、小车持续向左画圆且没有白灯停止，
 *   说明Hi3861仍在运行旧版双线程程序，并非本文件对应的新固件。
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "wifiiot_uart.h"
#include "wifiiot_errno.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "wifiiot_pwm.h"
#include "hi_pwm.h"
#include "hi_uart.h"
#include "wifiiot_gpio_ex.h"

#define PROTOCOL_MAX_SPEED_X100 250
#define DRIVE_SPEED_X100        250
#define TURN_SPEED_X100         60
#define COMMAND_REFRESH_MS      500U
#define PATH_START_DELAY_MS     3000U
#define PATH_STRAIGHT_MS        2000U
#define PATH_TURN_LEFT_MS       750U
#define PATH_TURN_RIGHT_MS      750U
#define PATH_STOP_MS            1000U
#define PATH_LOOP_PAUSE_MS      3000U

/***通信协议***/
/*
函数功能：发送至stm32的数据协议
参数    ： 电机实际转速的一百倍，例如：设置转速为1rad/s，则传入100
*/
static void stm32motor_control(int motorA, int motorB)
{
    uint8_t uart_sendbuf[6];
    uint8_t A_dir = 0;
    uint8_t B_dir = 0;
    int written;

    // 小车运动方向 前进（正转）：0   后退（反转）1
    if (motorA < 0) {
        A_dir = 1;
        motorA = -motorA;
    } else {
        A_dir = 0;
    }

    if (motorB < 0) {
        B_dir = 1;
        motorB = -motorB;
    } else {
        B_dir = 0;
    }

    // 速度字段为单字节，当前协议限制幅度为 -250 ~250
    if (motorA > PROTOCOL_MAX_SPEED_X100)
    {
        motorA = PROTOCOL_MAX_SPEED_X100;
    }

    if (motorB > PROTOCOL_MAX_SPEED_X100)
    {
        motorB = PROTOCOL_MAX_SPEED_X100;
    }

    // 数据协议
    uart_sendbuf[0] = 0xFC;      // 帧头
    uart_sendbuf[1] = A_dir;     // 左轮方向   0正转，1反转
    uart_sendbuf[2] = motorA;    // 左轮速度
    uart_sendbuf[3] = B_dir;     // 右轮方向   0正转，1反转
    uart_sendbuf[4] = motorB;    // 右轮速度
    uart_sendbuf[5] = 0xFD;      // 帧尾

    written = UartWrite(
        WIFI_IOT_UART_IDX_2,
        (unsigned char *)uart_sendbuf,
        6
    );
    if (written != (int)sizeof(uart_sendbuf)) {
        printf("UART2,WRITE_FAILED,%d\n", written);
    }
}

/*
 * 在指定时长内每500ms重发一次命令。
 * STM32端2s无有效帧会进入安全停车，因此路径运行期间不能只发送一次后长时间等待。
 */
static void hold_motor_command(int motorA, int motorB, unsigned int durationMs)
{
    unsigned int remainingMs = durationMs;

    while (remainingMs > 0U) {
        unsigned int stepMs = (remainingMs > COMMAND_REFRESH_MS) ?
            COMMAND_REFRESH_MS : remainingMs;

        stm32motor_control(motorA, motorB);
        usleep(stepMs * 1000U);
        remainingMs -= stepMs;
    }
}

/* 执行一个运动动作，并在动作后发送持续停车命令。 */
static void run_path_action(const char *name, int motorA, int motorB,
    unsigned int durationMs)
{
    printf("PATH,ACTION,%s\n", name);
    hold_motor_command(motorA, motorB, durationMs);
    printf("PATH,ACTION,%s,STOP\n", name);
    hold_motor_command(0, 0, PATH_STOP_MS);
}

/*
 * 综合闭合路径：前进两条边、后退两条边，并交替验证左右原地转向。
 * 每个转向时间独立定义，便于后续分别校准左右90度角。
 */
static void path_thread(void *arg)
{
    (void)arg;

    printf("PATH,READY,%u ms\n", PATH_START_DELAY_MS);
    hold_motor_command(0, 0, PATH_START_DELAY_MS);

    while (1) {
        printf("PATH,START\n");

        run_path_action("1_FORWARD", DRIVE_SPEED_X100, DRIVE_SPEED_X100,
            PATH_STRAIGHT_MS);
        run_path_action("2_TURN_LEFT", -TURN_SPEED_X100, TURN_SPEED_X100,
            PATH_TURN_LEFT_MS);
        run_path_action("3_FORWARD", DRIVE_SPEED_X100, DRIVE_SPEED_X100,
            PATH_STRAIGHT_MS);
        run_path_action("4_TURN_RIGHT", TURN_SPEED_X100, -TURN_SPEED_X100,
            PATH_TURN_RIGHT_MS);
        run_path_action("5_REVERSE", -DRIVE_SPEED_X100, -DRIVE_SPEED_X100,
            PATH_STRAIGHT_MS);
        run_path_action("6_TURN_LEFT", -TURN_SPEED_X100, TURN_SPEED_X100,
            PATH_TURN_LEFT_MS);
        run_path_action("7_REVERSE", -DRIVE_SPEED_X100, -DRIVE_SPEED_X100,
            PATH_STRAIGHT_MS);
        run_path_action("8_TURN_RIGHT", TURN_SPEED_X100, -TURN_SPEED_X100,
            PATH_TURN_RIGHT_MS);

        printf("PATH,COMPLETE\n");
        hold_motor_command(0, 0, PATH_LOOP_PAUSE_MS);
    }
}


/*****任务创建*****/
static void correspondence(void)
{
    osThreadAttr_t attr = {0};

    (void)GpioInit();  // GPIO功能初始化

    /********************通讯串口初始化********************/
    if (IoSetFunc(
            WIFI_IOT_IO_NAME_GPIO_11,
            WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) != WIFI_IOT_SUCCESS ||
        IoSetFunc(
            WIFI_IOT_IO_NAME_GPIO_12,
            WIFI_IOT_IO_FUNC_GPIO_12_UART2_RXD) != WIFI_IOT_SUCCESS) {
        printf("UART2,GPIO_INIT_FAILED\n");
        return;
    }

    /**************串口参数****************/
    WifiIotUartAttribute uart_attr2 = {
        // 波特率：115200
        .baudRate = 115200,

        // 数据位：8bits
        .dataBits = 8,

        .stopBits = 1,
        .parity = 0,
    };

    if (UartInit(WIFI_IOT_UART_IDX_2, &uart_attr2, NULL) != WIFI_IOT_SUCCESS) {
        printf("UART2,INIT_FAILED\n");
        return;
    }

    attr.stack_size = 1024 * 4;   // 任务栈大小

    // 只创建一个路径线程，保证UART命令顺序确定
    attr.name = "path_thread";    // 创建任务名称
    attr.priority = 25;           // 任务优先级

    if (osThreadNew(
            path_thread,
            NULL,
            &attr) == NULL)
    {
        printf("Failed to create path_thread!\n");
    }
}

APP_FEATURE_INIT(correspondence);  // 启动任务
