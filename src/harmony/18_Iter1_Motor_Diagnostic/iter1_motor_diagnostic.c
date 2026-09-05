#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

#define ITER1_MOTOR_UART WIFI_IOT_UART_IDX_2
#define ITER1_MOTOR_BAUD 115200U
#define ITER1_FRAME_SIZE 6U
#define ITER1_REFRESH_MS 100U
#define ITER1_SPEED 40U
#define ITER1_ACTION_MS 1200U
#define ITER1_STOP_MS 1200U
#define ITER1_FINAL_STOP_MS 3000U

typedef struct {
    const char *name;
    uint8_t leftDirection;
    uint8_t leftSpeed;
    uint8_t rightDirection;
    uint8_t rightSpeed;
    unsigned int durationMs;
} Iter1Action;

static const Iter1Action g_actions[] = {
    {"STOP_BEFORE", 0U, 0U, 0U, 0U, ITER1_STOP_MS},
    {"FORWARD", 0U, ITER1_SPEED, 0U, ITER1_SPEED, ITER1_ACTION_MS},
    {"STOP_AFTER_FORWARD", 0U, 0U, 0U, 0U, ITER1_STOP_MS},
    {"REVERSE", 1U, ITER1_SPEED, 1U, ITER1_SPEED, ITER1_ACTION_MS},
    {"STOP_AFTER_REVERSE", 0U, 0U, 0U, 0U, ITER1_STOP_MS},
    {"TURN_LEFT", 1U, ITER1_SPEED, 0U, ITER1_SPEED, ITER1_ACTION_MS},
    {"STOP_AFTER_LEFT", 0U, 0U, 0U, 0U, ITER1_STOP_MS},
    {"TURN_RIGHT", 0U, ITER1_SPEED, 1U, ITER1_SPEED, ITER1_ACTION_MS},
    {"STOP_AFTER_RIGHT", 0U, 0U, 0U, 0U, ITER1_STOP_MS},
    {"FINAL_STOP", 0U, 0U, 0U, 0U, ITER1_FINAL_STOP_MS},
};

static int SendFrame(const Iter1Action *action)
{
    uint8_t frame[ITER1_FRAME_SIZE] = {
        0xFCU,
        action->leftDirection,
        action->leftSpeed,
        action->rightDirection,
        action->rightSpeed,
        0xFDU,
    };
    int written = UartWrite(ITER1_MOTOR_UART, frame, sizeof(frame));

    if (written != (int)sizeof(frame)) {
        printf("ITER1,MOTOR_WRITE_ERROR,ACTION=%s,WRITTEN=%d\r\n",
            action->name, written);
        return -1;
    }
    return 0;
}

static void RunAction(const Iter1Action *action, unsigned int index)
{
    unsigned int elapsed = 0U;
    unsigned int frameCount = 0U;

    printf("ITER1,ACTION_START,INDEX=%u,NAME=%s,L_DIR=%u,L_SPEED=%u,"
           "R_DIR=%u,R_SPEED=%u,DURATION_MS=%u\r\n",
        index, action->name, action->leftDirection, action->leftSpeed,
        action->rightDirection, action->rightSpeed, action->durationMs);

    while (elapsed < action->durationMs) {
        if (SendFrame(action) != 0) {
            return;
        }
        ++frameCount;
        usleep(ITER1_REFRESH_MS * 1000U);
        elapsed += ITER1_REFRESH_MS;
    }

    printf("ITER1,ACTION_END,INDEX=%u,NAME=%s,FRAMES=%u\r\n",
        index, action->name, frameCount);
}

static void Iter1MotorDiagnosticTask(void *argument)
{
    unsigned int index;
    (void)argument;

    printf("ITER1,READY,MOTOR_DIAGNOSTIC,UART2_GPIO11_TX,115200,"
           "FRAME=FC_DIR_SPEED_DIR_SPEED_FD\r\n");
    printf("ITER1,SAFETY,WHEELS_MUST_BE_SUSPENDED,"
           "STOP_ON_UNEXPECTED_MOTION\r\n");

    for (index = 0U; index < sizeof(g_actions) / sizeof(g_actions[0]); ++index) {
        RunAction(&g_actions[index], index);
    }

    while (1) {
        (void)SendFrame(&g_actions[9]);
        usleep(ITER1_REFRESH_MS * 1000U);
    }
}

static void Iter1MotorDiagnosticEntry(void)
{
    const WifiIotUartAttribute attribute = {
        .baudRate = ITER1_MOTOR_BAUD,
        .dataBits = WIFI_IOT_UART_DATA_BIT_8,
        .stopBits = WIFI_IOT_UART_STOP_BIT_1,
        .parity = WIFI_IOT_UART_PARITY_NONE,
        .pad = 0U,
    };
    osThreadAttr_t threadAttribute = {0};

    if (GpioInit() != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11,
            WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) != WIFI_IOT_SUCCESS) {
        printf("ITER1,INIT_ERROR,UART2_GPIO11\r\n");
        return;
    }
    if (UartInit(ITER1_MOTOR_UART, &attribute, NULL) != WIFI_IOT_SUCCESS) {
        printf("ITER1,INIT_ERROR,UART2\r\n");
        return;
    }

    threadAttribute.name = "iter1_motor_diag";
    threadAttribute.stack_size = 4096U;
    threadAttribute.priority = osPriorityAboveNormal;
    if (osThreadNew(Iter1MotorDiagnosticTask, NULL, &threadAttribute) == NULL) {
        printf("ITER1,INIT_ERROR,TASK_CREATE\r\n");
    }
}

APP_FEATURE_INIT(Iter1MotorDiagnosticEntry);
