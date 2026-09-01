#include "car_uart.h"
#include "obstacle_controller.h"

#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

#define CAR_UART_INDEX WIFI_IOT_UART_IDX_1
#define CAR_UART_BAUD_RATE 2400U
#define CAR_FRAME_SYNC_1 0xA5U
#define CAR_FRAME_SYNC_2 0x5AU
#define CAR_FRAME_SIZE 5U
#define CAR_FRAME_REPEAT 3U

static uint8_t g_sequence;

static bool IsValidCommand(uint8_t command)
{
    return command == CAR_COMMAND_FORWARD || command == CAR_COMMAND_STOP ||
           command == CAR_COMMAND_TURN_LEFT ||
           command == CAR_COMMAND_TURN_RIGHT;
}

bool CarUart_Init(void)
{
    const WifiIotUartAttribute attribute = {
        .baudRate = CAR_UART_BAUD_RATE,
        .dataBits = WIFI_IOT_UART_DATA_BIT_8,
        .stopBits = WIFI_IOT_UART_STOP_BIT_1,
        .parity = WIFI_IOT_UART_PARITY_NONE,
        .pad = 0U,
    };
    const WifiIotUartExtraAttr extraAttribute = {
        .txFifoLine = WIFI_IOT_FIFO_LINE_ONE_EIGHT,
        .rxFifoLine = WIFI_IOT_FIFO_LINE_ONE_EIGHT,
        .flowFifoLine = WIFI_IOT_FIFO_LINE_ONE_EIGHT,
        .txBlock = WIFI_IOT_UART_BLOCK_STATE_BLOCK,
        .rxBlock = WIFI_IOT_UART_BLOCK_STATE_NONE_BLOCK,
        .txBufSize = 64U,
        .rxBufSize = 64U,
        .txUseDma = WIFI_IOT_UART_NONE_DMA,
        .rxUseDma = WIFI_IOT_UART_NONE_DMA,
    };

    (void)GpioInit();
    if (IoSetFunc(WIFI_IOT_IO_NAME_GPIO_6,
                  WIFI_IOT_IO_FUNC_GPIO_6_UART1_TXD) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_5,
                  WIFI_IOT_IO_FUNC_GPIO_5_UART1_RXD) != WIFI_IOT_SUCCESS ||
        IoSetPull(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_IO_PULL_UP) !=
            WIFI_IOT_SUCCESS) {
        return false;
    }

    g_sequence = 0U;
    return UartInit(CAR_UART_INDEX, &attribute, &extraAttribute) ==
           WIFI_IOT_SUCCESS;
}

bool CarUart_SendCommand(uint8_t command)
{
    uint8_t frame[CAR_FRAME_SIZE * CAR_FRAME_REPEAT];
    uint8_t checksum;
    unsigned int repeat;
    unsigned int offset;

    if (!IsValidCommand(command)) {
        return false;
    }

    checksum = (uint8_t)(CAR_FRAME_SYNC_1 ^ CAR_FRAME_SYNC_2 ^
                         g_sequence ^ command);
    for (repeat = 0U; repeat < CAR_FRAME_REPEAT; ++repeat) {
        offset = repeat * CAR_FRAME_SIZE;
        frame[offset] = CAR_FRAME_SYNC_1;
        frame[offset + 1U] = CAR_FRAME_SYNC_2;
        frame[offset + 2U] = g_sequence;
        frame[offset + 3U] = command;
        frame[offset + 4U] = checksum;
    }
    ++g_sequence;

    return UartWrite(CAR_UART_INDEX, frame, sizeof(frame)) ==
           (int)sizeof(frame);
}
