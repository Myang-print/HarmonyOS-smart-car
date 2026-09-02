#include "bluetooth_protocol.h"

#define DRIVE_SPEED_X100 100
#define REVERSE_SPEED_X100 150
#define TURN_INNER_SPEED_X100 50
#define TURN_OUTER_SPEED_X100 150

BluetoothByteResult BluetoothProtocol_ParseByte(uint8_t byte,
    CarAction *action)
{
    if (action == NULL) {
        return BLUETOOTH_BYTE_INVALID;
    }

    if (byte >= (uint8_t)'a' && byte <= (uint8_t)'z') {
        byte = (uint8_t)(byte - (uint8_t)('a' - 'A'));
    }

    switch (byte) {
        case 'W':
            *action = CAR_ACTION_FORWARD;
            break;
        case 'A':
            *action = CAR_ACTION_LEFT;
            break;
        case 'S':
            *action = CAR_ACTION_REVERSE;
            break;
        case 'D':
            *action = CAR_ACTION_RIGHT;
            break;
        case 'O':
            *action = CAR_ACTION_STOP;
            break;
        case '\r':
        case '\n':
        case ' ':
        case '\t':
            return BLUETOOTH_BYTE_IGNORED;
        default:
            *action = CAR_ACTION_STOP;
            return BLUETOOTH_BYTE_INVALID;
    }

    return BLUETOOTH_BYTE_ACCEPTED;
}

void BluetoothProtocol_GetMotorTarget(CarAction action, int *left,
    int *right)
{
    int leftTarget = 0;
    int rightTarget = 0;

    switch (action) {
        case CAR_ACTION_FORWARD:
            leftTarget = DRIVE_SPEED_X100;
            rightTarget = DRIVE_SPEED_X100;
            break;
        case CAR_ACTION_REVERSE:
            leftTarget = -REVERSE_SPEED_X100;
            rightTarget = -REVERSE_SPEED_X100;
            break;
        case CAR_ACTION_LEFT:
            leftTarget = -TURN_INNER_SPEED_X100;
            rightTarget = TURN_OUTER_SPEED_X100;
            break;
        case CAR_ACTION_RIGHT:
            leftTarget = TURN_OUTER_SPEED_X100;
            rightTarget = -TURN_INNER_SPEED_X100;
            break;
        default:
            break;
    }

    if (left != NULL) {
        *left = leftTarget;
    }
    if (right != NULL) {
        *right = rightTarget;
    }
}

static uint8_t EncodeSpeed(int speed, uint8_t *direction)
{
    if (speed < 0) {
        *direction = 1U;
        speed = -speed;
    } else {
        *direction = 0U;
    }

    if (speed > MOTOR_MAX_SPEED_X100) {
        speed = MOTOR_MAX_SPEED_X100;
    }
    return (uint8_t)speed;
}

void BluetoothProtocol_EncodeMotorFrame(int left, int right,
    uint8_t frame[MOTOR_FRAME_SIZE])
{
    uint8_t leftDirection;
    uint8_t rightDirection;

    if (frame == NULL) {
        return;
    }

    frame[0] = 0xFCU;
    frame[2] = EncodeSpeed(left, &leftDirection);
    frame[4] = EncodeSpeed(right, &rightDirection);
    frame[1] = leftDirection;
    frame[3] = rightDirection;
    frame[5] = 0xFDU;
}

const char *BluetoothProtocol_ActionName(CarAction action)
{
    switch (action) {
        case CAR_ACTION_FORWARD:
            return "FORWARD";
        case CAR_ACTION_REVERSE:
            return "REVERSE";
        case CAR_ACTION_LEFT:
            return "LEFT";
        case CAR_ACTION_RIGHT:
            return "RIGHT";
        default:
            return "STOP";
    }
}
