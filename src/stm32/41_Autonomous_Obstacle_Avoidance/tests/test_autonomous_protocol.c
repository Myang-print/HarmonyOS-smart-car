#include <assert.h>
#include <stdio.h>

#include "autonomous_protocol.h"

static void PushFrame(AutonomousProtocol *protocol, unsigned char sequence,
                      unsigned char command, unsigned long now_ms) {
  const unsigned char frame[] = {
      AUTONOMOUS_FRAME_SYNC_1,
      AUTONOMOUS_FRAME_SYNC_2,
      sequence,
      command,
      (unsigned char)(AUTONOMOUS_FRAME_SYNC_1 ^ AUTONOMOUS_FRAME_SYNC_2 ^
                      sequence ^ command)};
  unsigned int index;

  for (index = 0; index < sizeof(frame); index++)
    AutonomousProtocol_PushByte(protocol, frame[index], now_ms);
}

static void TestStartupIsSafeStop(void) {
  AutonomousProtocol protocol;

  AutonomousProtocol_Init(&protocol);
  assert(!AutonomousProtocol_HasLink(&protocol, 0UL));
  assert(AutonomousProtocol_GetSafeCommand(&protocol, 0UL) == 'S');
}

static void TestAllCommands(void) {
  const unsigned char commands[] = {'S', 'F', 'L', 'R'};
  unsigned int index;

  for (index = 0; index < sizeof(commands); index++) {
    AutonomousProtocol protocol;
    AutonomousProtocol_Init(&protocol);
    PushFrame(&protocol, (unsigned char)index, commands[index], 100UL);
    assert(AutonomousProtocol_HasLink(&protocol, 100UL));
    assert(AutonomousProtocol_GetSafeCommand(&protocol, 100UL) ==
           commands[index]);
  }
}

static void TestBadChecksumIsRejected(void) {
  AutonomousProtocol protocol;
  const unsigned char bad_frame[] = {0xA5U, 0x5AU, 7U, 'F', 0U};
  unsigned int index;

  AutonomousProtocol_Init(&protocol);
  for (index = 0; index < sizeof(bad_frame); index++)
    AutonomousProtocol_PushByte(&protocol, bad_frame[index], 20UL);
  assert(!AutonomousProtocol_HasLink(&protocol, 20UL));
  assert(AutonomousProtocol_GetSafeCommand(&protocol, 20UL) == 'S');
}

static void TestInvalidCommandIsRejected(void) {
  AutonomousProtocol protocol;

  AutonomousProtocol_Init(&protocol);
  PushFrame(&protocol, 8U, 'X', 50UL);
  assert(!AutonomousProtocol_HasLink(&protocol, 50UL));
}

static void TestNoiseAndResynchronization(void) {
  AutonomousProtocol protocol;
  const unsigned char noise[] = {0U, 0xA5U, 0x33U, 0xA5U};
  unsigned int index;

  AutonomousProtocol_Init(&protocol);
  for (index = 0; index < sizeof(noise); index++)
    AutonomousProtocol_PushByte(&protocol, noise[index], 10UL);
  AutonomousProtocol_PushByte(&protocol, 0x5AU, 10UL);
  AutonomousProtocol_PushByte(&protocol, 2U, 10UL);
  AutonomousProtocol_PushByte(&protocol, 'R', 10UL);
  AutonomousProtocol_PushByte(&protocol,
                              (unsigned char)(0xA5U ^ 0x5AU ^ 2U ^ 'R'),
                              10UL);
  assert(AutonomousProtocol_GetSafeCommand(&protocol, 10UL) == 'R');
}

static void TestTimeoutForcesStop(void) {
  AutonomousProtocol protocol;

  AutonomousProtocol_Init(&protocol);
  PushFrame(&protocol, 3U, 'F', 100UL);
  assert(AutonomousProtocol_HasLink(&protocol, 1100UL));
  assert(!AutonomousProtocol_HasLink(&protocol, 1101UL));
  assert(AutonomousProtocol_GetSafeCommand(&protocol, 1101UL) == 'S');
}

static void TestDuplicateFrameRefreshesHeartbeat(void) {
  AutonomousProtocol protocol;

  AutonomousProtocol_Init(&protocol);
  PushFrame(&protocol, 4U, 'L', 100UL);
  PushFrame(&protocol, 4U, 'L', 900UL);
  assert(AutonomousProtocol_HasLink(&protocol, 1500UL));
  assert(AutonomousProtocol_GetSafeCommand(&protocol, 1500UL) == 'L');
}

int main(void) {
  TestStartupIsSafeStop();
  TestAllCommands();
  TestBadChecksumIsRejected();
  TestInvalidCommandIsRejected();
  TestNoiseAndResynchronization();
  TestTimeoutForcesStop();
  TestDuplicateFrameRefreshesHeartbeat();
  puts("7 framed serial protocol tests passed");
  return 0;
}
