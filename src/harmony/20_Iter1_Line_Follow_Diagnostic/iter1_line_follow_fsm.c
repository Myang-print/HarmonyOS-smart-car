#include "iter1_line_follow_fsm.h"

static LineFsmCommand TrackCommand(int left, int right)
{
    if (left == 0 && right == 0) return LINE_FSM_WHEEL_FORWARD;
    if (left == 1 && right == 0) return LINE_FSM_WHEEL_LEFT_CORRECTION;
    if (left == 0 && right == 1) return LINE_FSM_WHEEL_RIGHT_CORRECTION;
    return LINE_FSM_WHEEL_STOP;
}

void LineFsm_Init(LineFsm *fsm)
{
    fsm->mode = LINE_FSM_TRACK;
    fsm->command = LINE_FSM_WHEEL_FORWARD;
    fsm->recovery_attempts = 0U;
    fsm->tick = 0U;
    fsm->valid_ticks = 0U;
    fsm->last_error = 0;
    fsm->has_valid_track = 0;
}

LineFsmCommand LineFsm_Step(LineFsm *fsm, int left, int right)
{
    LineFsmCommand tracked;
    ++fsm->tick;

    if (fsm->mode == LINE_FSM_STOP) {
        fsm->command = LINE_FSM_WHEEL_STOP;
        return fsm->command;
    }

    if (left < 0 || right < 0 || left > 1 || right > 1) {
        fsm->mode = LINE_FSM_STOP;
        fsm->command = LINE_FSM_WHEEL_STOP;
        fsm->last_error = -1;
        return fsm->command;
    }

    tracked = TrackCommand(left, right);
    if (tracked == LINE_FSM_WHEEL_STOP) {
        /* 11 is treated as an ambiguous edge condition, never as a stop. */
        if (!fsm->has_valid_track) {
            fsm->command = LINE_FSM_WHEEL_FORWARD;
        } else if (fsm->command == LINE_FSM_WHEEL_LEFT_CORRECTION ||
                   fsm->command == LINE_FSM_WHEEL_RIGHT_CORRECTION) {
            /* Continue the last forward-only correction. */
        } else {
            fsm->command = LINE_FSM_WHEEL_FORWARD;
        }
        fsm->mode = LINE_FSM_TRACK;
        return fsm->command;
    }

    fsm->has_valid_track = 1;
    fsm->valid_ticks = 0U;
    fsm->command = tracked;
    return fsm->command;
}

const char *LineFsm_ModeName(LineFsmMode mode)
{
    return mode == LINE_FSM_STOP ? "STOP" : "TRACK";
}

const char *LineFsm_CommandName(LineFsmCommand command)
{
    switch (command) {
        case LINE_FSM_WHEEL_FORWARD: return "FORWARD";
        case LINE_FSM_WHEEL_LEFT_CORRECTION: return "LEFT_CORRECTION";
        case LINE_FSM_WHEEL_RIGHT_CORRECTION: return "RIGHT_CORRECTION";
        case LINE_FSM_WHEEL_SPIN_LEFT: return "SPIN_LEFT_UNAVAILABLE";
        case LINE_FSM_WHEEL_SPIN_RIGHT: return "SPIN_RIGHT_UNAVAILABLE";
        default: return "STOP";
    }
}
