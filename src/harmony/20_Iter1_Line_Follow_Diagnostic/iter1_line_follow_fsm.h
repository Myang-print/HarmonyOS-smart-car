#ifndef ITER1_LINE_FOLLOW_FSM_H
#define ITER1_LINE_FOLLOW_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LINE_FSM_TRACK = 0,
    LINE_FSM_SPIN,
    LINE_FSM_CONFIRM_FORWARD,
    LINE_FSM_STOP
} LineFsmMode;

typedef enum {
    LINE_FSM_WHEEL_STOP = 0,
    LINE_FSM_WHEEL_FORWARD,
    LINE_FSM_WHEEL_LEFT_CORRECTION,
    LINE_FSM_WHEEL_RIGHT_CORRECTION,
    LINE_FSM_WHEEL_SPIN_LEFT,
    LINE_FSM_WHEEL_SPIN_RIGHT
} LineFsmCommand;

typedef struct {
    LineFsmMode mode;
    LineFsmCommand command;
    unsigned int recovery_attempts;
    unsigned int tick;
    unsigned int valid_ticks;
    int last_error;
    int has_valid_track;
} LineFsm;

void LineFsm_Init(LineFsm *fsm);
LineFsmCommand LineFsm_Step(LineFsm *fsm, int left, int right);
const char *LineFsm_ModeName(LineFsmMode mode);
const char *LineFsm_CommandName(LineFsmCommand command);

#ifdef __cplusplus
}
#endif
#endif
