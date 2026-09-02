#ifndef BLUETOOTH_PROTOCOL_H
#define BLUETOOTH_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define MOTOR_FRAME_SIZE 6U
#define MOTOR_MAX_SPEED_X100 250

typedef enum {
    CAR_ACTION_STOP = 0,
    CAR_ACTION_FORWARD,
    CAR_ACTION_REVERSE,
    CAR_ACTION_LEFT,
    CAR_ACTION_RIGHT
} CarAction;

typedef enum {
    BLUETOOTH_BYTE_IGNORED = 0,
    BLUETOOTH_BYTE_ACCEPTED,
    BLUETOOTH_BYTE_INVALID
} BluetoothByteResult;

BluetoothByteResult BluetoothProtocol_ParseByte(uint8_t byte,
    CarAction *action);
void BluetoothProtocol_GetMotorTarget(CarAction action, int *left,
    int *right);
void BluetoothProtocol_EncodeMotorFrame(int left, int right,
    uint8_t frame[MOTOR_FRAME_SIZE]);
const char *BluetoothProtocol_ActionName(CarAction action);

#endif
