#include "obstacle_controller.h"

#include <stddef.h>

static void EnterState(ObstacleController *controller, ObstacleState state,
                       uint32_t nowMs)
{
    controller->state = state;
    controller->stateStartedMs = nowMs;
}

static uint32_t NextRandom(ObstacleController *controller)
{
    uint32_t value = controller->randomState;

    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    controller->randomState = value;
    return value;
}

static ObstacleState ChooseTurn(ObstacleController *controller)
{
    return ((NextRandom(controller) & 1U) == 0U)
               ? OBSTACLE_STATE_TURN_LEFT
               : OBSTACLE_STATE_TURN_RIGHT;
}

void ObstacleController_Init(ObstacleController *controller,
                             const ObstacleConfig *config,
                             uint32_t nowMs, uint32_t randomSeed)
{
    if ((controller == NULL) || (config == NULL)) {
        return;
    }

    controller->config = *config;
    controller->randomState = (randomSeed == 0U) ? 0x6D2B79F5U : randomSeed;
    controller->sensorFaultStop = true;
    EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
}

uint8_t ObstacleController_Update(ObstacleController *controller,
                                  uint32_t nowMs, bool distanceValid,
                                  float distanceCm)
{
    uint32_t elapsed;

    if (controller == NULL) {
        return CAR_COMMAND_STOP;
    }

    if (!distanceValid) {
        /* A missing or stale sensor reading is a safety fault, never a clear path. */
        if (!controller->sensorFaultStop ||
            (controller->state != OBSTACLE_STATE_STOP)) {
            EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
        }
        controller->sensorFaultStop = true;
        return CAR_COMMAND_STOP;
    }

    elapsed = nowMs - controller->stateStartedMs;
    switch (controller->state) {
        case OBSTACLE_STATE_FORWARD:
            if (distanceCm < controller->config.obstacleThresholdCm) {
                EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
                controller->sensorFaultStop = false;
                return CAR_COMMAND_STOP;
            }
            return CAR_COMMAND_FORWARD;

        case OBSTACLE_STATE_STOP:
            if (controller->sensorFaultStop) {
                controller->sensorFaultStop = false;
                if (distanceCm >= controller->config.clearThresholdCm) {
                    EnterState(controller, OBSTACLE_STATE_FORWARD, nowMs);
                    return CAR_COMMAND_FORWARD;
                }
                EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
                return CAR_COMMAND_STOP;
            }
            if (elapsed >= controller->config.stopDurationMs) {
                EnterState(controller, ChooseTurn(controller), nowMs);
                return (controller->state == OBSTACLE_STATE_TURN_LEFT)
                           ? CAR_COMMAND_TURN_LEFT
                           : CAR_COMMAND_TURN_RIGHT;
            }
            return CAR_COMMAND_STOP;

        case OBSTACLE_STATE_TURN_LEFT:
        case OBSTACLE_STATE_TURN_RIGHT:
            if (elapsed >= controller->config.turnDurationMs) {
                if (distanceCm < controller->config.clearThresholdCm) {
                    EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
                    controller->sensorFaultStop = false;
                    return CAR_COMMAND_STOP;
                }
                EnterState(controller, OBSTACLE_STATE_FORWARD, nowMs);
                return CAR_COMMAND_FORWARD;
            }
            return (controller->state == OBSTACLE_STATE_TURN_LEFT)
                       ? CAR_COMMAND_TURN_LEFT
                       : CAR_COMMAND_TURN_RIGHT;

        default:
            EnterState(controller, OBSTACLE_STATE_STOP, nowMs);
            return CAR_COMMAND_STOP;
    }
}

const char *ObstacleController_StateName(ObstacleState state)
{
    switch (state) {
        case OBSTACLE_STATE_FORWARD:
            return "FORWARD";
        case OBSTACLE_STATE_STOP:
            return "STOP";
        case OBSTACLE_STATE_TURN_LEFT:
            return "TURN_LEFT";
        case OBSTACLE_STATE_TURN_RIGHT:
            return "TURN_RIGHT";
        default:
            return "UNKNOWN";
    }
}
