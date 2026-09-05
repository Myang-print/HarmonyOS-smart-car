#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

#define FAILSAFE_UART WIFI_IOT_UART_IDX_2
#define FAILSAFE_BAUD 115200U

static void FailsafeTask(void *argument)
{
    const uint8_t frame[6] = {0xFCU, 0U, 40U, 0U, 40U, 0xFDU};
    int written;
    (void)argument;

    written = UartWrite(FAILSAFE_UART, frame, sizeof(frame));
    printf("ITER1,FAILSAFE,FORWARD_ONCE,WRITTEN=%d\r\n", written);
    printf("ITER1,FAILSAFE,SILENCE_NOW,WAIT_4_SECONDS\r\n");
    sleep(4U);
    printf("ITER1,FAILSAFE,OBSERVATION_WINDOW_END\r\n");
    while (1) {
        sleep(1U);
    }
}

static void FailsafeEntry(void)
{
    const WifiIotUartAttribute attribute = {
        .baudRate = FAILSAFE_BAUD,
        .dataBits = WIFI_IOT_UART_DATA_BIT_8,
        .stopBits = WIFI_IOT_UART_STOP_BIT_1,
        .parity = WIFI_IOT_UART_PARITY_NONE,
        .pad = 0U,
    };
    osThreadAttr_t taskAttribute = {0};

    if (GpioInit() != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11,
            WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) != WIFI_IOT_SUCCESS ||
        UartInit(FAILSAFE_UART, &attribute, NULL) != WIFI_IOT_SUCCESS) {
        printf("ITER1,FAILSAFE,INIT_ERROR\r\n");
        return;
    }
    taskAttribute.name = "iter1_failsafe_diag";
    taskAttribute.stack_size = 2048U;
    taskAttribute.priority = osPriorityAboveNormal;
    if (osThreadNew(FailsafeTask, NULL, &taskAttribute) == NULL) {
        printf("ITER1,FAILSAFE,TASK_CREATE_ERROR\r\n");
    }
}

APP_FEATURE_INIT(FailsafeEntry);
