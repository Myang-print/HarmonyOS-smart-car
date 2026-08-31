#ifndef __AUTONOMOUS_PROTOCOL_H
#define __AUTONOMOUS_PROTOCOL_H

#define AUTONOMOUS_FRAME_SYNC_1 0xA5U
#define AUTONOMOUS_FRAME_SYNC_2 0x5AU
#define AUTONOMOUS_FRAME_SIZE 5U
#define AUTONOMOUS_LINK_TIMEOUT_MS 1000UL

#define AUTONOMOUS_COMMAND_FORWARD 'F'
#define AUTONOMOUS_COMMAND_STOP 'S'
#define AUTONOMOUS_COMMAND_LEFT 'L'
#define AUTONOMOUS_COMMAND_RIGHT 'R'

typedef enum {
  AUTONOMOUS_RX_WAIT_SYNC_1 = 0,
  AUTONOMOUS_RX_WAIT_SYNC_2,
  AUTONOMOUS_RX_SEQUENCE,
  AUTONOMOUS_RX_COMMAND,
  AUTONOMOUS_RX_CHECKSUM
} AutonomousRxState;

typedef struct {
  AutonomousRxState rx_state;
  unsigned char sequence;
  unsigned char pending_command;
  unsigned char checksum;
  unsigned char command;
  unsigned char has_valid_frame;
  unsigned long last_valid_frame_ms;
} AutonomousProtocol;

void AutonomousProtocol_Init(AutonomousProtocol *protocol);
void AutonomousProtocol_PushByte(AutonomousProtocol *protocol,
                                 unsigned char byte,
                                 unsigned long now_ms);
unsigned char AutonomousProtocol_GetSafeCommand(
    const AutonomousProtocol *protocol, unsigned long now_ms);
unsigned char AutonomousProtocol_HasLink(const AutonomousProtocol *protocol,
                                         unsigned long now_ms);

#endif
