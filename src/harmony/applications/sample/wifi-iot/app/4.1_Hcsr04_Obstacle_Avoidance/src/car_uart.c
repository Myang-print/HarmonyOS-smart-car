#include "car_uart.h"

#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

#define CAR_MODE0_GPIO WIFI_IOT_GPIO_IDX_6
#define CAR_MODE1_GPIO WIFI_IOT_GPIO_IDX_5

static bool ConfigureOutput(WifiIotGpioIdx gpio, WifiIotIoName io,
                            unsigned char function)
{
    if (IoSetFunc(io, function) != WIFI_IOT_SUCCESS) {
        return false;
    }
    return GpioSetDir(gpio, WIFI_IOT_GPIO_DIR_OUT) == WIFI_IOT_SUCCESS;
}

bool CarUart_Init(void)
{
    (void)GpioInit();
    if (!ConfigureOutput(CAR_MODE0_GPIO, WIFI_IOT_IO_NAME_GPIO_6,
                         WIFI_IOT_IO_FUNC_GPIO_6_GPIO) ||
        !ConfigureOutput(CAR_MODE1_GPIO, WIFI_IOT_IO_NAME_GPIO_5,
                         WIFI_IOT_IO_FUNC_GPIO_5_GPIO)) {
        return false;
    }

    (void)GpioSetOutputVal(CAR_MODE0_GPIO, WIFI_IOT_GPIO_VALUE0);
    (void)GpioSetOutputVal(CAR_MODE1_GPIO, WIFI_IOT_GPIO_VALUE0);
    return true;
}

bool CarUart_SendCommand(uint8_t command)
{
    uint8_t mode;
    WifiIotGpioValue mode0;
    WifiIotGpioValue mode1;

    switch (command) {
        case 'F': mode = 1U; break;
        case 'L': mode = 2U; break;
        case 'R': mode = 3U; break;
        case 'S': mode = 0U; break;
        default: return false;
    }

    mode0 = (mode & 0x01U) ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    mode1 = (mode & 0x02U) ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    if (GpioSetOutputVal(CAR_MODE0_GPIO, mode0) != WIFI_IOT_SUCCESS ||
        GpioSetOutputVal(CAR_MODE1_GPIO, mode1) != WIFI_IOT_SUCCESS) {
        return false;
    }

    return true;
}
