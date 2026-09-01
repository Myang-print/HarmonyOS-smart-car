#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "car_uart.h"
#include "cmsis_os2.h"
#include "hcsr04_driver.h"
#include "hi_time.h"
#include "obstacle_controller.h"
#include "ohos_init.h"
#include "wifiiot_watchdog.h"

#define OBSTACLE_THRESHOLD_CM 20.0F
#define CLEAR_THRESHOLD_CM 25.0F
#define STOP_DURATION_MS 500U
/* STM32 needs at most 1.66 s to brake, reverse, turn and probe. */
#define TURN_INTENT_HOLD_MS 2000U
#define CONTROL_IDLE_MS 20U
#define LOG_PERIOD_MS 1000U
#define TRANSIENT_FAILURE_LIMIT 1U
#define TASK_STACK_SIZE (4U * 1024U)

static uint32_t GetMilliseconds(void)
{
    return hi_get_us() / 1000U;
}

static void DelayMilliseconds(uint32_t milliseconds)
{
    uint32_t ticks = (milliseconds * osKernelGetTickFreq() + 999U) / 1000U;
    osDelay((ticks == 0U) ? 1U : ticks);
}

static void ObstacleTask(void *argument)
{
    const ObstacleConfig config = {
        OBSTACLE_THRESHOLD_CM, CLEAR_THRESHOLD_CM,
        STOP_DURATION_MS, TURN_INTENT_HOLD_MS
    };
    ObstacleController controller;
    ObstacleState previousState = OBSTACLE_STATE_STOP;
    float distanceCm = 0.0F;
    uint32_t nowMs = GetMilliseconds();
    uint32_t lastLogMs = nowMs - LOG_PERIOD_MS;
    uint8_t previousCommand = 0U;
    uint8_t invalidStreak = 0U;
    uint8_t command;
    bool valid;
    bool decisionValid;
    bool hasValidDistance = false;
    (void)argument;

    ObstacleController_Init(&controller, &config, nowMs,
                            hi_get_us() ^ 0xA341316CU);
    printf("obstacle controller ready: UART1_TX GPIO6, 2400-8-N-1\r\n");

    while (1) {
        valid = Hcsr04_ReadMedian(&distanceCm);
        nowMs = GetMilliseconds();
        if (valid) {
            hasValidDistance = true;
            invalidStreak = 0U;
        } else if (invalidStreak < UINT8_MAX) {
            ++invalidStreak;
        }
        decisionValid = valid ||
                        (hasValidDistance &&
                         (invalidStreak <= TRANSIENT_FAILURE_LIMIT));
        command = ObstacleController_Update(&controller, nowMs, decisionValid,
                                             distanceCm);

        if (!CarUart_SendCommand(command)) {
            printf("car framed UART link write failed\r\n");
            (void)CarUart_SendCommand(CAR_COMMAND_STOP);
        }

        if ((command != previousCommand) ||
            (controller.state != previousState) ||
            ((uint32_t)(nowMs - lastLogMs) >= LOG_PERIOD_MS)) {
            printf("obstacle state=%s distance=%s%.1fcm command=%c link=UART1\r\n",
                   ObstacleController_StateName(controller.state),
                   valid ? "" : (decisionValid ? "REUSED/" : "INVALID/"),
                   distanceCm, command);
            previousCommand = command;
            previousState = controller.state;
            lastLogMs = nowMs;
        }
        DelayMilliseconds(CONTROL_IDLE_MS);
    }
}

static void Hcsr04ObstacleApp(void)
{
    osThreadAttr_t taskAttr = {0};

    WatchDogDisable();
    if (!Hcsr04_Init()) {
        printf("HC-SR04 initialization failed\r\n");
        return;
    }
    if (!CarUart_Init()) {
        printf("car framed UART link initialization failed\r\n");
        return;
    }

    /* No movement is allowed before the first valid distance decision. */
    (void)CarUart_SendCommand(CAR_COMMAND_STOP);

    taskAttr.name = "obstacle_control";
    taskAttr.stack_size = TASK_STACK_SIZE;
    taskAttr.priority = osPriorityAboveNormal;
    if (osThreadNew(ObstacleTask, NULL, &taskAttr) == NULL) {
        printf("obstacle task creation failed\r\n");
        (void)CarUart_SendCommand(CAR_COMMAND_STOP);
    }
}

APP_FEATURE_INIT(Hcsr04ObstacleApp);
