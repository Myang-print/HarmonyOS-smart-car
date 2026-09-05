#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"
#include "iter1_line_follow_fsm.h"

#define LINE_LEFT_GPIO 13
#define LINE_RIGHT_GPIO 14
#define LINE_UART WIFI_IOT_UART_IDX_2
#define LINE_BAUD 115200U
#define LINE_PERIOD_MS 100U
#define LINE_SPEED_FAST 100U
#define LINE_SPEED_SLOW 45U

static int ReadLine(WifiIotGpioValue *left, WifiIotGpioValue *right)
{
    if (GpioGetInputVal(LINE_LEFT_GPIO, left) != WIFI_IOT_SUCCESS ||
        GpioGetInputVal(LINE_RIGHT_GPIO, right) != WIFI_IOT_SUCCESS) {
        return -1;
    }
    return 0;
}

static int SendWheels(uint8_t leftDirection, uint8_t leftSpeed,
    uint8_t rightDirection, uint8_t rightSpeed)
{
    const uint8_t frame[6] = {
        0xFCU, leftDirection, leftSpeed, rightDirection, rightSpeed, 0xFDU
    };
    return UartWrite(LINE_UART, frame, sizeof(frame)) == (int)sizeof(frame)
        ? 0 : -1;
}

static void SendCommand(LineFsmCommand command)
{
    switch (command) {
        case LINE_FSM_WHEEL_FORWARD:
            (void)SendWheels(0U, LINE_SPEED_FAST, 0U, LINE_SPEED_FAST);
            break;
        case LINE_FSM_WHEEL_LEFT_CORRECTION:
            (void)SendWheels(0U, LINE_SPEED_SLOW, 0U, LINE_SPEED_FAST);
            break;
        case LINE_FSM_WHEEL_RIGHT_CORRECTION:
            (void)SendWheels(0U, LINE_SPEED_FAST, 0U, LINE_SPEED_SLOW);
            break;
        case LINE_FSM_WHEEL_SPIN_LEFT:
        case LINE_FSM_WHEEL_SPIN_RIGHT:
            /* 保留枚举兼容性，但本算法不允许反向或原地旋转。 */
            (void)SendWheels(0U, LINE_SPEED_FAST, 0U, LINE_SPEED_FAST);
            break;
        default:
            (void)SendWheels(0U, 0U, 0U, 0U);
            break;
    }
}

static void LineFollowTask(void *argument)
{
    WifiIotGpioValue left = WIFI_IOT_GPIO_VALUE0;
    WifiIotGpioValue right = WIFI_IOT_GPIO_VALUE0;
    LineFsm fsm;
    unsigned long sample = 0U;
    int readResult;
    LineFsmCommand command;
    (void)argument;

    LineFsm_Init(&fsm);
    printf("ITER1,READY,LINE_FOLLOW_FSM,UART2_GPIO11_TX,115200\r\n");
    printf("ITER1,SAFETY,FORWARD_ONLY_EDGE_TRACKING,11_KEEP_LAST_FORWARD,"
           "NO_REVERSE_NO_SPIN,STOP_ON_SENSOR_ERROR\r\n");

    while (1) {
        readResult = ReadLine(&left, &right);
        ++sample;
        if (readResult != 0) {
            fsm.mode = LINE_FSM_STOP;
            fsm.command = LINE_FSM_WHEEL_STOP;
            command = LINE_FSM_WHEEL_STOP;
            SendCommand(command);
            printf("ITER1,LINE,N=%lu,L=?,R=?,MODE=STOP,CMD=STOP\r\n",
                sample);
        } else {
            command = LineFsm_Step(&fsm, (int)left, (int)right);
            SendCommand(command);
            printf("ITER1,LINE,N=%lu,L=%u,R=%u,MODE=%s,CMD=%s,"
                   "RECOVERY_TICKS=%u\r\n",
                sample, (unsigned int)left, (unsigned int)right,
                LineFsm_ModeName(fsm.mode), LineFsm_CommandName(command),
                fsm.recovery_attempts);
        }
        usleep(LINE_PERIOD_MS * 1000U);
    }
}

static void LineFollowEntry(void)
{
    const WifiIotUartAttribute attribute = {
        .baudRate = LINE_BAUD,
        .dataBits = WIFI_IOT_UART_DATA_BIT_8,
        .stopBits = WIFI_IOT_UART_STOP_BIT_1,
        .parity = WIFI_IOT_UART_PARITY_NONE,
        .pad = 0U,
    };
    osThreadAttr_t taskAttribute = {0};

    if (GpioInit() != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13,
            WIFI_IOT_IO_FUNC_GPIO_13_GPIO) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14,
            WIFI_IOT_IO_FUNC_GPIO_14_GPIO) != WIFI_IOT_SUCCESS ||
        GpioSetDir(LINE_LEFT_GPIO, WIFI_IOT_GPIO_DIR_IN) != WIFI_IOT_SUCCESS ||
        GpioSetDir(LINE_RIGHT_GPIO, WIFI_IOT_GPIO_DIR_IN) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11,
            WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) != WIFI_IOT_SUCCESS ||
        UartInit(LINE_UART, &attribute, NULL) != WIFI_IOT_SUCCESS) {
        printf("ITER1,LINE,INIT_ERROR\r\n");
        return;
    }

    (void)SendWheels(0U, 0U, 0U, 0U);
    taskAttribute.name = "iter1_line_follow_fsm";
    taskAttribute.stack_size = 4096U;
    taskAttribute.priority = osPriorityAboveNormal;
    if (osThreadNew(LineFollowTask, NULL, &taskAttribute) == NULL) {
        printf("ITER1,LINE,TASK_CREATE_ERROR\r\n");
    }
}

APP_FEATURE_INIT(LineFollowEntry);
