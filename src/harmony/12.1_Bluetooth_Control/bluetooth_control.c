#include <stdio.h>
#include <unistd.h>

#include "bluetooth_protocol.h"
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

#define BLUETOOTH_UART_INDEX WIFI_IOT_UART_IDX_1
#define MOTOR_UART_INDEX WIFI_IOT_UART_IDX_2
#define BLUETOOTH_UART_BAUD_RATE 9600U
#define MOTOR_UART_BAUD_RATE 115200U
#define MOTOR_REFRESH_MS 100U
#define BLUETOOTH_POLL_MS 50U
#define BLUETOOTH_RX_BUFFER_SIZE 1000U

static volatile CarAction g_carAction = CAR_ACTION_STOP;
static uint8_t g_bluetoothBuffer[BLUETOOTH_RX_BUFFER_SIZE];

static int InitUart(WifiIotUartIdx index, unsigned int baudRate)
{
    const WifiIotUartAttribute attribute = {
        .baudRate = baudRate,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

    /* 与已在同型号小车验证成功的工程保持一致，不覆盖驱动默认扩展属性。 */
    return UartInit(index, &attribute, NULL);
}

static void SendMotorAction(CarAction action)
{
    uint8_t frame[MOTOR_FRAME_SIZE];
    int left;
    int right;
    int written;

    BluetoothProtocol_GetMotorTarget(action, &left, &right);
    BluetoothProtocol_EncodeMotorFrame(left, right, frame);
    written = UartWrite(MOTOR_UART_INDEX, frame, sizeof(frame));
    if (written != (int)sizeof(frame)) {
        printf("BTCTRL,MOTOR_UART_WRITE_FAILED,%d\n", written);
    }
}

static void BluetoothRxTask(void *argument)
{
    CarAction nextAction;
    BluetoothByteResult result;
    int count;
    int index;

    (void)argument;
    while (1) {
        count = UartRead(BLUETOOTH_UART_INDEX, g_bluetoothBuffer,
            sizeof(g_bluetoothBuffer));
        if (count > 0) {
            printf("BTCTRL,BLE_RX,%d", count);
            for (index = 0; index < count; index++) {
                printf(",%02X", g_bluetoothBuffer[index]);
                nextAction = g_carAction;
                result = BluetoothProtocol_ParseByte(g_bluetoothBuffer[index],
                    &nextAction);
                if (result == BLUETOOTH_BYTE_ACCEPTED) {
                    g_carAction = nextAction;
                    printf("[%s]", BluetoothProtocol_ActionName(nextAction));
                }
            }
            printf("\n");
            g_bluetoothBuffer[0] = 0U;
        }
        usleep(BLUETOOTH_POLL_MS * 1000U);
    }
}

static void MotorTxTask(void *argument)
{
    CarAction lastAction = CAR_ACTION_STOP;
    CarAction action;

    (void)argument;
    while (1) {
        action = g_carAction;
        SendMotorAction(action);
        if (action != lastAction) {
            printf("BTCTRL,ACTION,%s\n",
                BluetoothProtocol_ActionName(action));
            lastAction = action;
        }
        usleep(MOTOR_REFRESH_MS * 1000U);
    }
}

static void BluetoothControlEntry(void)
{
    osThreadAttr_t rxAttribute = {0};
    osThreadAttr_t txAttribute = {0};

    (void)GpioInit();
    g_carAction = CAR_ACTION_STOP;

    /* 成功基线的初始化顺序：先蓝牙UART1，再车控UART2。 */
    if (IoSetFunc(WIFI_IOT_IO_NAME_GPIO_0,
            WIFI_IOT_IO_FUNC_GPIO_0_UART1_TXD) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_1,
            WIFI_IOT_IO_FUNC_GPIO_1_UART1_RXD) != WIFI_IOT_SUCCESS) {
        printf("BTCTRL,UART1_GPIO_INIT_FAILED\n");
        return;
    }
    if (InitUart(BLUETOOTH_UART_INDEX, BLUETOOTH_UART_BAUD_RATE) !=
        WIFI_IOT_SUCCESS) {
        printf("BTCTRL,UART1_INIT_FAILED\n");
        return;
    }

    if (IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11,
            WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) != WIFI_IOT_SUCCESS ||
        IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12,
            WIFI_IOT_IO_FUNC_GPIO_12_UART2_RXD) != WIFI_IOT_SUCCESS) {
        printf("BTCTRL,UART2_GPIO_INIT_FAILED\n");
        return;
    }
    if (InitUart(MOTOR_UART_INDEX, MOTOR_UART_BAUD_RATE) !=
        WIFI_IOT_SUCCESS) {
        printf("BTCTRL,UART2_INIT_FAILED\n");
        return;
    }

    /*
     * 必须先创建车控发送线程，并让它的优先级高于阻塞式UART1接收。
     * 否则UartRead可能在高优先级线程内长期占用执行权，STM32收不到心跳。
     */
    txAttribute.name = "motor_tx";
    txAttribute.stack_size = 2048U;
    txAttribute.priority = 25;
    if (osThreadNew(MotorTxTask, NULL, &txAttribute) == NULL) {
        printf("BTCTRL,CREATE_TX_TASK_FAILED\n");
        return;
    }

    rxAttribute.name = "bluetooth_rx";
    rxAttribute.stack_size = 4096U;
    rxAttribute.priority = 24;
    if (osThreadNew(BluetoothRxTask, NULL, &rxAttribute) == NULL) {
        printf("BTCTRL,CREATE_RX_TASK_FAILED,MOTOR_STOP_HEARTBEAT_ACTIVE\n");
        return;
    }

    printf("BTCTRL,READY,BLE=UART1_GPIO0_1_9600,"
        "MOTOR=UART2_GPIO11_12_115200\n");
}

APP_FEATURE_INIT(BluetoothControlEntry);
