#include <assert.h>
#include <stdio.h>

#include "autonomous_behavior.h"
#include "autonomous_protocol.h"

static void TestClearPathMovesForward(void) {
  AutonomousBehavior behavior;
  AutonomousBehavior_Init(&behavior, 0U);
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_FORWARD, 0U) ==
         AUTONOMOUS_MOTION_FORWARD);
}

static void TestCompleteLeftAvoidanceSequence(void) {
  AutonomousBehavior behavior;
  unsigned long now = 0U;

  AutonomousBehavior_Init(&behavior, now);
  (void)AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_FORWARD, now);
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, now) ==
         AUTONOMOUS_MOTION_BRAKE);
  now += AUTONOMOUS_BRAKE_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, now) ==
         AUTONOMOUS_MOTION_REVERSE);
  now += AUTONOMOUS_REVERSE_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, now) ==
         AUTONOMOUS_MOTION_BRAKE);
  now += AUTONOMOUS_DIRECTION_PAUSE_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, now) ==
         AUTONOMOUS_MOTION_LEFT);
  now += AUTONOMOUS_TURN_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, now) ==
         AUTONOMOUS_MOTION_PROBE);
  now += AUTONOMOUS_PROBE_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_FORWARD, now) ==
         AUTONOMOUS_MOTION_FORWARD);
}

static void TestStopAbortsEveryPhase(void) {
  AutonomousBehavior behavior;

  AutonomousBehavior_Init(&behavior, 0U);
  (void)AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT, 0U);
  (void)AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_LEFT,
                                  AUTONOMOUS_BRAKE_MS);
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_STOP,
                                   AUTONOMOUS_BRAKE_MS + 1U) ==
         AUTONOMOUS_MOTION_STOP);
}

static void TestRightIntentIsLatched(void) {
  AutonomousBehavior behavior;
  unsigned long now = 0U;

  AutonomousBehavior_Init(&behavior, now);
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_RIGHT, now) ==
         AUTONOMOUS_MOTION_REVERSE);
  now += AUTONOMOUS_REVERSE_MS;
  (void)AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_FORWARD, now);
  now += AUTONOMOUS_DIRECTION_PAUSE_MS;
  assert(AutonomousBehavior_Update(&behavior, AUTONOMOUS_COMMAND_FORWARD, now) ==
         AUTONOMOUS_MOTION_RIGHT);
}

int main(void) {
  TestClearPathMovesForward();
  TestCompleteLeftAvoidanceSequence();
  TestStopAbortsEveryPhase();
  TestRightIntentIsLatched();
  puts("4 autonomous-behavior tests passed");
  return 0;
}
