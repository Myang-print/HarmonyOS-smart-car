#include "autonomous_behavior.h"

#include "autonomous_protocol.h"

static void EnterState(AutonomousBehavior *behavior,
                       AutonomousBehaviorState state,
                       unsigned long now_ms) {
  behavior->state = state;
  behavior->state_started_ms = now_ms;
}

static unsigned char IsTurnCommand(unsigned char command) {
  return command == AUTONOMOUS_COMMAND_LEFT ||
         command == AUTONOMOUS_COMMAND_RIGHT;
}

static AutonomousMotion MotionForState(const AutonomousBehavior *behavior) {
  switch (behavior->state) {
  case AUTONOMOUS_BEHAVIOR_FORWARD:
    return AUTONOMOUS_MOTION_FORWARD;
  case AUTONOMOUS_BEHAVIOR_BRAKE:
  case AUTONOMOUS_BEHAVIOR_DIRECTION_PAUSE:
  case AUTONOMOUS_BEHAVIOR_VERIFY_STOP:
    return AUTONOMOUS_MOTION_BRAKE;
  case AUTONOMOUS_BEHAVIOR_REVERSE:
    return AUTONOMOUS_MOTION_REVERSE;
  case AUTONOMOUS_BEHAVIOR_TURN_LEFT:
    return AUTONOMOUS_MOTION_LEFT;
  case AUTONOMOUS_BEHAVIOR_TURN_RIGHT:
    return AUTONOMOUS_MOTION_RIGHT;
  case AUTONOMOUS_BEHAVIOR_PROBE:
    return AUTONOMOUS_MOTION_PROBE;
  default:
    return AUTONOMOUS_MOTION_STOP;
  }
}

void AutonomousBehavior_Init(AutonomousBehavior *behavior,
                             unsigned long now_ms) {
  if (behavior == 0)
    return;

  behavior->pending_turn = AUTONOMOUS_COMMAND_LEFT;
  EnterState(behavior, AUTONOMOUS_BEHAVIOR_STOP, now_ms);
}

AutonomousMotion AutonomousBehavior_Update(AutonomousBehavior *behavior,
                                           unsigned char command,
                                           unsigned long now_ms) {
  unsigned long elapsed;

  if (behavior == 0)
    return AUTONOMOUS_MOTION_STOP;

  /* STOP is an emergency command and may abort every avoidance phase. */
  if (command == AUTONOMOUS_COMMAND_STOP) {
    if (behavior->state != AUTONOMOUS_BEHAVIOR_STOP)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_STOP, now_ms);
    return AUTONOMOUS_MOTION_STOP;
  }

  elapsed = now_ms - behavior->state_started_ms;
  switch (behavior->state) {
  case AUTONOMOUS_BEHAVIOR_STOP:
    if (command == AUTONOMOUS_COMMAND_FORWARD)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_FORWARD, now_ms);
    else if (IsTurnCommand(command)) {
      behavior->pending_turn = command;
      /* Hi3861 holds STOP for 500 ms before sending the turn intent. */
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_REVERSE, now_ms);
    }
    break;

  case AUTONOMOUS_BEHAVIOR_FORWARD:
    if (IsTurnCommand(command)) {
      behavior->pending_turn = command;
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_BRAKE, now_ms);
    }
    break;

  case AUTONOMOUS_BEHAVIOR_BRAKE:
    if (elapsed >= AUTONOMOUS_BRAKE_MS)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_REVERSE, now_ms);
    break;

  case AUTONOMOUS_BEHAVIOR_REVERSE:
    if (elapsed >= AUTONOMOUS_REVERSE_MS)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_DIRECTION_PAUSE, now_ms);
    break;

  case AUTONOMOUS_BEHAVIOR_DIRECTION_PAUSE:
    if (elapsed >= AUTONOMOUS_DIRECTION_PAUSE_MS) {
      EnterState(behavior,
                 behavior->pending_turn == AUTONOMOUS_COMMAND_LEFT
                     ? AUTONOMOUS_BEHAVIOR_TURN_LEFT
                     : AUTONOMOUS_BEHAVIOR_TURN_RIGHT,
                 now_ms);
    }
    break;

  case AUTONOMOUS_BEHAVIOR_TURN_LEFT:
  case AUTONOMOUS_BEHAVIOR_TURN_RIGHT:
    if (elapsed >= AUTONOMOUS_TURN_MS)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_PROBE, now_ms);
    break;

  case AUTONOMOUS_BEHAVIOR_PROBE:
    if (elapsed >= AUTONOMOUS_PROBE_MS) {
      if (command == AUTONOMOUS_COMMAND_FORWARD)
        EnterState(behavior, AUTONOMOUS_BEHAVIOR_FORWARD, now_ms);
      else
        EnterState(behavior, AUTONOMOUS_BEHAVIOR_VERIFY_STOP, now_ms);
    }
    break;

  case AUTONOMOUS_BEHAVIOR_VERIFY_STOP:
    if (command == AUTONOMOUS_COMMAND_FORWARD)
      EnterState(behavior, AUTONOMOUS_BEHAVIOR_FORWARD, now_ms);
    break;

  default:
    EnterState(behavior, AUTONOMOUS_BEHAVIOR_STOP, now_ms);
    break;
  }

  return MotionForState(behavior);
}
