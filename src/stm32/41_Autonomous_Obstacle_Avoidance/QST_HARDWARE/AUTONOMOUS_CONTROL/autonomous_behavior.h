#ifndef __AUTONOMOUS_BEHAVIOR_H
#define __AUTONOMOUS_BEHAVIOR_H

#define AUTONOMOUS_BRAKE_MS 360U
#define AUTONOMOUS_REVERSE_MS 450U
#define AUTONOMOUS_DIRECTION_PAUSE_MS 100U
#define AUTONOMOUS_TURN_MS 500U
#define AUTONOMOUS_PROBE_MS 250U

typedef enum {
  AUTONOMOUS_MOTION_STOP = 0,
  AUTONOMOUS_MOTION_BRAKE,
  AUTONOMOUS_MOTION_FORWARD,
  AUTONOMOUS_MOTION_REVERSE,
  AUTONOMOUS_MOTION_LEFT,
  AUTONOMOUS_MOTION_RIGHT,
  AUTONOMOUS_MOTION_PROBE
} AutonomousMotion;

typedef enum {
  AUTONOMOUS_BEHAVIOR_STOP = 0,
  AUTONOMOUS_BEHAVIOR_FORWARD,
  AUTONOMOUS_BEHAVIOR_BRAKE,
  AUTONOMOUS_BEHAVIOR_REVERSE,
  AUTONOMOUS_BEHAVIOR_DIRECTION_PAUSE,
  AUTONOMOUS_BEHAVIOR_TURN_LEFT,
  AUTONOMOUS_BEHAVIOR_TURN_RIGHT,
  AUTONOMOUS_BEHAVIOR_PROBE,
  AUTONOMOUS_BEHAVIOR_VERIFY_STOP
} AutonomousBehaviorState;

typedef struct {
  AutonomousBehaviorState state;
  unsigned char pending_turn;
  unsigned long state_started_ms;
} AutonomousBehavior;

void AutonomousBehavior_Init(AutonomousBehavior *behavior,
                             unsigned long now_ms);
AutonomousMotion AutonomousBehavior_Update(AutonomousBehavior *behavior,
                                           unsigned char command,
                                           unsigned long now_ms);

#endif
