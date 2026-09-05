#include <assert.h>
#include <stdio.h>

#define CENTERED 1
#define LEFT_LINE 2
#define RIGHT_LINE 3
#define UNCERTAIN 4

static int classify(int left, int right)
{
    if (left == 0 && right == 0) return CENTERED;
    if (left == 1 && right == 0) return LEFT_LINE;
    if (left == 0 && right == 1) return RIGHT_LINE;
    return UNCERTAIN;
}

int main(void)
{
    assert(classify(0, 0) == CENTERED);
    assert(classify(1, 0) == LEFT_LINE);
    assert(classify(0, 1) == RIGHT_LINE);
    assert(classify(1, 1) == UNCERTAIN);
    puts("iter1_line_follow_classification_test: PASS");
    return 0;
}
