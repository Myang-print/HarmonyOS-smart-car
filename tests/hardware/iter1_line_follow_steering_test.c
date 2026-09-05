#include <assert.h>
#include <stdio.h>

static const int kFast = 100;
static const int kSlow = 45;

static void expected_wheels(int left, int right, int *out_left, int *out_right)
{
    if (left == 0 && right == 0) {
        *out_left = kFast;
        *out_right = kFast;
    } else if (left == 1 && right == 0) {
        *out_left = kSlow;
        *out_right = kFast;
    } else if (left == 0 && right == 1) {
        *out_left = kFast;
        *out_right = kSlow;
    } else {
        *out_left = 0;
        *out_right = 0;
    }
}

int main(void)
{
    int left;
    int right;

    expected_wheels(0, 0, &left, &right);
    assert(left == kFast && right == kFast);
    expected_wheels(1, 0, &left, &right);
    assert(left == kSlow && right == kFast);
    expected_wheels(0, 1, &left, &right);
    assert(left == kFast && right == kSlow);
    expected_wheels(1, 1, &left, &right);
    assert(left == 0 && right == 0);
    puts("iter1_line_follow_steering_test: PASS");
    return 0;
}
