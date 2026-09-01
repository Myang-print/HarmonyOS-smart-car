#ifndef OBSTACLE_CONTROLLER_H
#define OBSTACLE_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OBSTACLE_STATE_FORWARD = 0,
    OBSTACLE_STATE_STOP,
    OBSTACLE_STATE_TURN_LEFT,
    OBSTACLE_STATE_TURN_RIGHT
} ObstacleState;

typedef struct {
    float obstacleThresholdCm;
    float clearThresholdCm;
    uint32_t stopDurationMs;
    uint32_t turnDurationMs;
} ObstacleConfig;

typedef struct {
    ObstacleState state;
    uint32_t stateStartedMs;
    uint32_t randomState;
    bool sensorFaultStop;
    ObstacleConfig config;
} ObstacleController;

#define CAR_COMMAND_FORWARD ((uint8_t)'F')
#define CAR_COMMAND_STOP ((uint8_t)'S')
#define CAR_COMMAND_TURN_LEFT ((uint8_t)'L')
#define CAR_COMMAND_TURN_RIGHT ((uint8_t)'R')

void ObstacleController_Init(ObstacleController *controller,
                             const ObstacleConfig *config,
                             uint32_t nowMs, uint32_t randomSeed);
uint8_t ObstacleController_Update(ObstacleController *controller,
                                  uint32_t nowMs, bool distanceValid,
                                  float distanceCm);
const char *ObstacleController_StateName(ObstacleState state);

#endif
