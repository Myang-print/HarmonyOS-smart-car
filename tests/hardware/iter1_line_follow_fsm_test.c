#include <assert.h>
#include <stdio.h>

#include "iter1_line_follow_fsm.h"

static void test_normal_tracking_is_forward_only(void)
{
    LineFsm fsm;
    LineFsm_Init(&fsm);
    assert(LineFsm_Step(&fsm, 0, 0) == LINE_FSM_WHEEL_FORWARD);
    assert(LineFsm_Step(&fsm, 1, 0) == LINE_FSM_WHEEL_LEFT_CORRECTION);
    assert(LineFsm_Step(&fsm, 0, 1) == LINE_FSM_WHEEL_RIGHT_CORRECTION);
}

static void test_11_keeps_last_forward_command(void)
{
    LineFsm fsm;
    LineFsm_Init(&fsm);
    assert(LineFsm_Step(&fsm, 0, 0) == LINE_FSM_WHEEL_FORWARD);
    assert(LineFsm_Step(&fsm, 1, 1) == LINE_FSM_WHEEL_FORWARD);
    assert(LineFsm_Step(&fsm, 1, 1) == LINE_FSM_WHEEL_FORWARD);
}

static void test_11_keeps_last_left_correction(void)
{
    LineFsm fsm;
    LineFsm_Init(&fsm);
    assert(LineFsm_Step(&fsm, 1, 0) == LINE_FSM_WHEEL_LEFT_CORRECTION);
    assert(LineFsm_Step(&fsm, 1, 1) == LINE_FSM_WHEEL_LEFT_CORRECTION);
}

static void test_11_keeps_last_right_correction(void)
{
    LineFsm fsm;
    LineFsm_Init(&fsm);
    assert(LineFsm_Step(&fsm, 0, 1) == LINE_FSM_WHEEL_RIGHT_CORRECTION);
    assert(LineFsm_Step(&fsm, 1, 1) == LINE_FSM_WHEEL_RIGHT_CORRECTION);
}

static void test_no_reverse_or_spin_commands(void)
{
    LineFsm fsm;
    unsigned int i;
    LineFsm_Init(&fsm);
    for (i = 0; i < 30U; ++i) {
        LineFsmCommand command = LineFsm_Step(&fsm, 1, 1);
        assert(command != LINE_FSM_WHEEL_SPIN_LEFT);
        assert(command != LINE_FSM_WHEEL_SPIN_RIGHT);
        assert(command != LINE_FSM_WHEEL_STOP);
    }
    assert(fsm.mode == LINE_FSM_TRACK);
}

static void test_sensor_error_stops(void)
{
    LineFsm fsm;
    LineFsm_Init(&fsm);
    assert(LineFsm_Step(&fsm, -1, -1) == LINE_FSM_WHEEL_STOP);
    assert(fsm.mode == LINE_FSM_STOP);
}

int main(void)
{
    test_normal_tracking_is_forward_only();
    test_11_keeps_last_forward_command();
    test_11_keeps_last_left_correction();
    test_11_keeps_last_right_correction();
    test_no_reverse_or_spin_commands();
    test_sensor_error_stops();
    puts("iter1_line_follow_forward_edge_test: PASS");
    return 0;
}
