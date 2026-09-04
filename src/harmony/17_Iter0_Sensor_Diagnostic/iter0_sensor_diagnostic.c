#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

#define ITER0_LEFT_SENSOR_GPIO 13
#define ITER0_RIGHT_SENSOR_GPIO 14
#define ITER0_SAMPLE_PERIOD_MS 100U

static int ReadSensor(int gpio, WifiIotGpioValue *value)
{
    if (value == NULL) {
        return -1;
    }
    return GpioGetInputVal(gpio, value);
}

static void Iter0SensorDiagnosticTask(void *argument)
{
    WifiIotGpioValue left = WIFI_IOT_GPIO_VALUE0;
    WifiIotGpioValue right = WIFI_IOT_GPIO_VALUE0;
    unsigned long sample = 0U;
    int leftResult;
    int rightResult;

    (void)argument;
    printf("ITER0,READY,SENSOR_DIAGNOSTIC,GPIO13=LEFT,GPIO14=RIGHT\r\n");
    printf("ITER0,INSTRUCTION,DO_NOT_MOVE_MOTORS,PLACE_SENSORS_OVER_WHITE_OR_BLACK\r\n");

    while (1) {
        leftResult = ReadSensor(ITER0_LEFT_SENSOR_GPIO, &left);
        rightResult = ReadSensor(ITER0_RIGHT_SENSOR_GPIO, &right);
        ++sample;
        if (leftResult != WIFI_IOT_SUCCESS || rightResult != WIFI_IOT_SUCCESS) {
            printf("ITER0,SENSOR_READ_ERROR,N=%lu,L_RC=%d,R_RC=%d\r\n",
                sample, leftResult, rightResult);
        } else {
            printf("ITER0,SENSOR,N=%lu,L=%u,R=%u\r\n",
                sample, (unsigned int)left, (unsigned int)right);
        }
        usleep(ITER0_SAMPLE_PERIOD_MS * 1000U);
    }
}

static void Iter0SensorDiagnosticEntry(void)
{
    osThreadAttr_t attribute = {0};

    if (GpioInit() != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13,
            WIFI_IOT_IO_FUNC_GPIO_13_GPIO) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14,
            WIFI_IOT_IO_FUNC_GPIO_14_GPIO) != WIFI_IOT_SUCCESS ||
        GpioSetDir(ITER0_LEFT_SENSOR_GPIO, WIFI_IOT_GPIO_DIR_IN) !=
            WIFI_IOT_SUCCESS ||
        GpioSetDir(ITER0_RIGHT_SENSOR_GPIO, WIFI_IOT_GPIO_DIR_IN) !=
            WIFI_IOT_SUCCESS) {
        printf("ITER0,INIT_ERROR,SENSOR_GPIO\r\n");
        return;
    }

    attribute.name = "iter0_sensor_diag";
    attribute.stack_size = 2048U;
    attribute.priority = osPriorityNormal;
    if (osThreadNew(Iter0SensorDiagnosticTask, NULL, &attribute) == NULL) {
        printf("ITER0,INIT_ERROR,TASK_CREATE\r\n");
    }
}

APP_FEATURE_INIT(Iter0SensorDiagnosticEntry);
