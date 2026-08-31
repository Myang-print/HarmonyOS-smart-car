#include "autonomous_protocol.h"

static unsigned char IsValidCommand(unsigned char command) {
  return command == AUTONOMOUS_COMMAND_FORWARD ||
         command == AUTONOMOUS_COMMAND_STOP ||
         command == AUTONOMOUS_COMMAND_LEFT ||
         command == AUTONOMOUS_COMMAND_RIGHT;
}

static void RestartParser(AutonomousProtocol *protocol, unsigned char byte) {
  if (byte == AUTONOMOUS_FRAME_SYNC_1) {
    protocol->rx_state = AUTONOMOUS_RX_WAIT_SYNC_2;
    protocol->checksum = AUTONOMOUS_FRAME_SYNC_1;
  } else {
    protocol->rx_state = AUTONOMOUS_RX_WAIT_SYNC_1;
    protocol->checksum = 0U;
  }
}

void AutonomousProtocol_Init(AutonomousProtocol *protocol) {
  if (protocol == 0)
    return;

  protocol->rx_state = AUTONOMOUS_RX_WAIT_SYNC_1;
  protocol->sequence = 0U;
  protocol->pending_command = AUTONOMOUS_COMMAND_STOP;
  protocol->checksum = 0U;
  protocol->command = AUTONOMOUS_COMMAND_STOP;
  protocol->has_valid_frame = 0U;
  protocol->last_valid_frame_ms = 0UL;
}

void AutonomousProtocol_PushByte(AutonomousProtocol *protocol,
                                 unsigned char byte,
                                 unsigned long now_ms) {
  if (protocol == 0)
    return;

  switch (protocol->rx_state) {
  case AUTONOMOUS_RX_WAIT_SYNC_1:
    RestartParser(protocol, byte);
    break;

  case AUTONOMOUS_RX_WAIT_SYNC_2:
    if (byte == AUTONOMOUS_FRAME_SYNC_2) {
      protocol->checksum ^= byte;
      protocol->rx_state = AUTONOMOUS_RX_SEQUENCE;
    } else {
      RestartParser(protocol, byte);
    }
    break;

  case AUTONOMOUS_RX_SEQUENCE:
    protocol->sequence = byte;
    protocol->checksum ^= byte;
    protocol->rx_state = AUTONOMOUS_RX_COMMAND;
    break;

  case AUTONOMOUS_RX_COMMAND:
    protocol->pending_command = byte;
    protocol->checksum ^= byte;
    protocol->rx_state = AUTONOMOUS_RX_CHECKSUM;
    break;

  case AUTONOMOUS_RX_CHECKSUM:
    if (byte == protocol->checksum &&
        IsValidCommand(protocol->pending_command)) {
      protocol->command = protocol->pending_command;
      protocol->last_valid_frame_ms = now_ms;
      protocol->has_valid_frame = 1U;
    }
    RestartParser(protocol, byte);
    break;

  default:
    RestartParser(protocol, byte);
    break;
  }
}

unsigned char AutonomousProtocol_HasLink(const AutonomousProtocol *protocol,
                                         unsigned long now_ms) {
  if (protocol == 0 || !protocol->has_valid_frame)
    return 0U;
  return (unsigned long)(now_ms - protocol->last_valid_frame_ms) <=
         AUTONOMOUS_LINK_TIMEOUT_MS;
}

unsigned char AutonomousProtocol_GetSafeCommand(
    const AutonomousProtocol *protocol, unsigned long now_ms) {
  if (!AutonomousProtocol_HasLink(protocol, now_ms))
    return AUTONOMOUS_COMMAND_STOP;
  return protocol->command;
}
