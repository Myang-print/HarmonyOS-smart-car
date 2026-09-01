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
#define TURN_DURATION_MS 300U
#define SENSOR_STALE_MS 500U
#define CONTROL_PERIOD_MS 20U
#define COMMAND_REFRESH_MS 100U
#define LOG_PERIOD_MS 1000U
#define TASK_STACK_SIZE (4U * 1024U)

typedef struct {
    float distanceCm;
    uint32_t measuredMs;
    bool valid;
} DistanceSnapshot;

static DistanceSnapshot g_distance;
static osMutexId_t g_distanceMutex;

static uint32_t GetMilliseconds(void)
{
    return hi_get_us() / 1000U;
}

static void DelayMilliseconds(uint32_t milliseconds)
{
    uint32_t ticks = (milliseconds * osKernelGetTickFreq() + 999U) / 1000U;
    osDelay((ticks == 0U) ? 1U : ticks);
}

static void SensorTask(void *argument)
{
    DistanceSnapshot measured = {0};
    (void)argument;

    while (1) {
        measured.valid = Hcsr04_ReadMedian(&measured.distanceCm);
        measured.measuredMs = GetMilliseconds();

        (void)osMutexAcquire(g_distanceMutex, osWaitForever);
        g_distance = measured;
        (void)osMutexRelease(g_distanceMutex);
    }
}

static void ControlTask(void *argument)
{
    const ObstacleConfig config = {
        OBSTACLE_THRESHOLD_CM, CLEAR_THRESHOLD_CM,
        STOP_DURATION_MS, TURN_DURATION_MS
    };
    ObstacleController controller;
    DistanceSnapshot snapshot;
    ObstacleState previousState = OBSTACLE_STATE_STOP;
    uint32_t nowMs = GetMilliseconds();
    uint32_t lastCommandMs = nowMs - COMMAND_REFRESH_MS;
    uint32_t lastLogMs = nowMs - LOG_PERIOD_MS;
    uint8_t previousCommand = 0U;
    uint8_t command;
    bool fresh;
    (void)argument;

    ObstacleController_Init(&controller, &config, nowMs,
                            hi_get_us() ^ 0xA341316CU);

    while (1) {
        nowMs = GetMilliseconds();
        (void)osMutexAcquire(g_distanceMutex, osWaitForever);
        snapshot = g_distance;
        (void)osMutexRelease(g_distanceMutex);

        fresh = snapshot.valid &&
                ((uint32_t)(nowMs - snapshot.measuredMs) <= SENSOR_STALE_MS);
        command = ObstacleController_Update(&controller, nowMs, fresh,
                                             snapshot.distanceCm);

        if ((command != previousCommand) ||
            ((uint32_t)(nowMs - lastCommandMs) >= COMMAND_REFRESH_MS)) {
            if (!CarUart_SendCommand(command)) {
                printf("car GPIO link write failed\r\n");
            }
            previousCommand = command;
            lastCommandMs = nowMs;
        }

        if ((controller.state != previousState) ||
            ((uint32_t)(nowMs - lastLogMs) >= LOG_PERIOD_MS)) {
            printf("obstacle state=%s distance=%s%.1fcm command=%c gpio=%u%u\r\n",
                   ObstacleController_StateName(controller.state),
                   fresh ? "" : "INVALID/", snapshot.distanceCm, command,
                   (command == CAR_COMMAND_TURN_LEFT ||
                    command == CAR_COMMAND_TURN_RIGHT) ? 1U : 0U,
                   (command == CAR_COMMAND_FORWARD ||
                    command == CAR_COMMAND_TURN_RIGHT) ? 1U : 0U);
            previousState = controller.state;
            lastLogMs = nowMs;
        }
        DelayMilliseconds(CONTROL_PERIOD_MS);
    }
}

static void Hcsr04ObstacleApp(void)
{
    osThreadAttr_t sensorAttr = {0};
    osThreadAttr_t controlAttr = {0};

    WatchDogDisable();
    g_distanceMutex = osMutexNew(NULL);
    if (g_distanceMutex == NULL) {
        printf("distance mutex creation failed\r\n");
        return;
    }
    if (!Hcsr04_Init()) {
        printf("HC-SR04 initialization failed\r\n");
        return;
    }
    if (!CarUart_Init()) {
        printf("car GPIO link initialization failed\r\n");
        return;
    }

    /* Keep the STM32 stopped until the first valid distance is available. */
    (void)CarUart_SendCommand(CAR_COMMAND_STOP);

    sensorAttr.name = "hcsr04_sensor";
    sensorAttr.stack_size = TASK_STACK_SIZE;
    sensorAttr.priority = osPriorityNormal;
    controlAttr.name = "obstacle_control";
    controlAttr.stack_size = TASK_STACK_SIZE;
    controlAttr.priority = osPriorityAboveNormal;

    if (osThreadNew(SensorTask, NULL, &sensorAttr) == NULL) {
        printf("sensor task creation failed\r\n");
        return;
    }
    if (osThreadNew(ControlTask, NULL, &controlAttr) == NULL) {
        printf("control task creation failed\r\n");
        (void)CarUart_SendCommand(CAR_COMMAND_STOP);
    }
}

APP_FEATURE_INIT(Hcsr04ObstacleApp);
