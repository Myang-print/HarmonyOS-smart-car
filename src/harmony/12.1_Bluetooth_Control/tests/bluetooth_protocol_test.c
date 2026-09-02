#include <assert.h>
#include <stdio.h>

#include "../bluetooth_protocol.h"

static void AssertFrame(CarAction action, const uint8_t expected[6])
{
    uint8_t actual[6];
    int left;
    int right;
    unsigned int index;

    BluetoothProtocol_GetMotorTarget(action, &left, &right);
    BluetoothProtocol_EncodeMotorFrame(left, right, actual);
    for (index = 0; index < 6U; index++) {
        assert(actual[index] == expected[index]);
    }
}

int main(void)
{
    CarAction action = CAR_ACTION_STOP;
    const uint8_t forward[6] = {0xFC, 0, 100, 0, 100, 0xFD};
    const uint8_t reverse[6] = {0xFC, 1, 150, 1, 150, 0xFD};
    const uint8_t left[6] = {0xFC, 1, 50, 0, 150, 0xFD};
    const uint8_t right[6] = {0xFC, 0, 150, 1, 50, 0xFD};
    const uint8_t stop[6] = {0xFC, 0, 0, 0, 0, 0xFD};

    assert(BluetoothProtocol_ParseByte('W', &action) ==
        BLUETOOTH_BYTE_ACCEPTED && action == CAR_ACTION_FORWARD);
    assert(BluetoothProtocol_ParseByte('a', &action) ==
        BLUETOOTH_BYTE_ACCEPTED && action == CAR_ACTION_LEFT);
    assert(BluetoothProtocol_ParseByte('S', &action) ==
        BLUETOOTH_BYTE_ACCEPTED && action == CAR_ACTION_REVERSE);
    assert(BluetoothProtocol_ParseByte('d', &action) ==
        BLUETOOTH_BYTE_ACCEPTED && action == CAR_ACTION_RIGHT);
    assert(BluetoothProtocol_ParseByte('O', &action) ==
        BLUETOOTH_BYTE_ACCEPTED && action == CAR_ACTION_STOP);
    assert(BluetoothProtocol_ParseByte('\r', &action) ==
        BLUETOOTH_BYTE_IGNORED && action == CAR_ACTION_STOP);
    action = CAR_ACTION_FORWARD;
    assert(BluetoothProtocol_ParseByte('X', &action) ==
        BLUETOOTH_BYTE_INVALID && action == CAR_ACTION_STOP);

    AssertFrame(CAR_ACTION_FORWARD, forward);
    AssertFrame(CAR_ACTION_REVERSE, reverse);
    AssertFrame(CAR_ACTION_LEFT, left);
    AssertFrame(CAR_ACTION_RIGHT, right);
    AssertFrame(CAR_ACTION_STOP, stop);

    puts("bluetooth_protocol_test: PASS");
    return 0;
}
