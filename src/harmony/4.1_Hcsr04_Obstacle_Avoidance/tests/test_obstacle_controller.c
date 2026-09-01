#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "obstacle_controller.h"

static const ObstacleConfig CONFIG = {20.0F, 25.0F, 500U, 300U};

static ObstacleController NewController(uint32_t nowMs, uint32_t seed)
{
    ObstacleController controller;
    ObstacleController_Init(&controller, &CONFIG, nowMs, seed);
    return controller;
}

static void TestInitialStateIsSafeStop(void)
{
    ObstacleController controller = NewController(100U, 1U);
    assert(controller.state == OBSTACLE_STATE_STOP);
    assert(ObstacleController_Update(&controller, 100U, false, 0.0F) ==
           CAR_COMMAND_STOP);
}

static void TestClearReadingRecoversFaultStopImmediately(void)
{
    ObstacleController controller = NewController(0U, 1U);
    (void)ObstacleController_Update(&controller, 600U, false, 0.0F);
    assert(ObstacleController_Update(&controller, 1099U, true, 50.0F) ==
           CAR_COMMAND_FORWARD);
    assert(controller.state == OBSTACLE_STATE_FORWARD);
}

static void TestNearReadingAfterFaultStartsStopDelay(void)
{
    ObstacleController controller = NewController(0U, 1U);
    (void)ObstacleController_Update(&controller, 100U, false, 0.0F);
    assert(ObstacleController_Update(&controller, 200U, true, 10.0F) ==
           CAR_COMMAND_STOP);
    assert(ObstacleController_Update(&controller, 699U, true, 10.0F) ==
           CAR_COMMAND_STOP);
    assert(ObstacleController_Update(&controller, 700U, true, 10.0F) !=
           CAR_COMMAND_STOP);
}

static void TestStopThenForwardOnClearPath(void)
{
    ObstacleController controller = NewController(0U, 1U);
    (void)ObstacleController_Update(&controller, 0U, true, 10.0F);
    uint8_t command = ObstacleController_Update(&controller, 500U, true, 50.0F);
    assert((command == CAR_COMMAND_TURN_LEFT) ||
           (command == CAR_COMMAND_TURN_RIGHT));
    assert(ObstacleController_Update(&controller, 799U, true, 50.0F) == command);
    assert(ObstacleController_Update(&controller, 800U, true, 50.0F) ==
           CAR_COMMAND_FORWARD);
    assert(controller.state == OBSTACLE_STATE_FORWARD);
}

static void TestNearObstacleStopsImmediately(void)
{
    ObstacleController controller = NewController(0U, 1U);
    controller.state = OBSTACLE_STATE_FORWARD;
    controller.sensorFaultStop = false;
    assert(ObstacleController_Update(&controller, 10U, true, 19.9F) ==
           CAR_COMMAND_STOP);
    assert(controller.state == OBSTACLE_STATE_STOP);
}

static void TestThresholdBoundaryIsClear(void)
{
    ObstacleController controller = NewController(0U, 1U);
    controller.state = OBSTACLE_STATE_FORWARD;
    controller.sensorFaultStop = false;
    assert(ObstacleController_Update(&controller, 10U, true, 20.0F) ==
           CAR_COMMAND_FORWARD);
}

static void TestFirstValidReadingAtTwentyTwoCmStartsForward(void)
{
    ObstacleController controller = NewController(0U, 1U);
    (void)ObstacleController_Update(&controller, 0U, false, 0.0F);
    assert(ObstacleController_Update(&controller, 10U, true, 22.0F) ==
           CAR_COMMAND_FORWARD);
    assert(controller.state == OBSTACLE_STATE_FORWARD);
}

static void TestPersistentObstacleRestopsAfterTurn(void)
{
    ObstacleController controller = NewController(0U, 2U);
    (void)ObstacleController_Update(&controller, 0U, true, 10.0F);
    (void)ObstacleController_Update(&controller, 500U, true, 10.0F);
    assert(ObstacleController_Update(&controller, 800U, true, 10.0F) ==
           CAR_COMMAND_STOP);
    assert(controller.state == OBSTACLE_STATE_STOP);
}

static void TestTimeCounterWraparound(void)
{
    ObstacleController controller = NewController(UINT32_MAX - 100U, 3U);
    controller.sensorFaultStop = false;
    assert(ObstacleController_Update(&controller, 398U, true, 50.0F) ==
           CAR_COMMAND_STOP);
    assert(ObstacleController_Update(&controller, 399U, true, 50.0F) !=
           CAR_COMMAND_STOP);
}

static void TestRandomStrategyUsesBothDirections(void)
{
    uint32_t seed;
    unsigned int left = 0U;
    unsigned int right = 0U;

    for (seed = 1U; seed <= 128U; ++seed) {
        ObstacleController controller = NewController(0U, seed);
        (void)ObstacleController_Update(&controller, 0U, true, 10.0F);
        uint8_t command = ObstacleController_Update(&controller, 500U, true, 10.0F);
        left += (command == CAR_COMMAND_TURN_LEFT) ? 1U : 0U;
        right += (command == CAR_COMMAND_TURN_RIGHT) ? 1U : 0U;
    }
    assert(left > 40U);
    assert(right > 40U);
}

int main(void)
{
    TestInitialStateIsSafeStop();
    TestClearReadingRecoversFaultStopImmediately();
    TestNearReadingAfterFaultStartsStopDelay();
    TestStopThenForwardOnClearPath();
    TestNearObstacleStopsImmediately();
    TestThresholdBoundaryIsClear();
    TestFirstValidReadingAtTwentyTwoCmStartsForward();
    TestPersistentObstacleRestopsAfterTurn();
    TestTimeCounterWraparound();
    TestRandomStrategyUsesBothDirections();
    puts("10 obstacle-controller tests passed");
    return 0;
}
