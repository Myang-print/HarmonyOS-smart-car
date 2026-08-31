# STM32 autonomous motor controller

This build replaces the ambiguous static two-GPIO mode with a framed one-way
serial link. It uses the existing Hi3861 GPIO6-to-STM32 PB8 wire, so no rewiring
is required.

## Link and wiring

- Hi3861 GPIO6 (UART1 TX) -> STM32 PB8 (TIM3-sampled software UART RX)
- Hi3861, STM32, and motor-driver logic grounds must be connected
- The old GPIO5 -> PB9 wire may remain connected but is ignored
- Serial format: 2400 baud, 8 data bits, no parity, 1 stop bit
- Frame: `A5 5A sequence command xor`
- Commands: `F=forward`, `S=stop`, `L=left`, `R=right`

PB8 uses an internal pull-up because UART idle is high. EXTI8 detects a start
bit and TIM3 samples the eight data bits and stop bit without blocking the
20 ms motor-control loop. TIM4 remains dedicated to motor PWM.

Only a complete frame with a valid command and XOR checksum establishes the
link. Duplicate frames refresh the heartbeat. If no valid frame arrives for
1000 ms, the command becomes STOP and the link-fault light is shown. Therefore
a disconnected wire can no longer be mistaken for a valid STOP command.

## Indicators

- Flashing red: no valid frame yet, or valid-frame timeout
- Dim amber: valid link with an explicit STOP command
- Blue/white: forward or low-speed probe
- Blue gradient: left spin
- Red gradient: right spin

WS2812 output is deferred until the receiver has been quiet for a control-loop
period. The Hi3861 repeats each frame three times, so a rare collision between
the first frame and LED bit-banging can recover on a later copy.

## Avoidance sequence

The Hi3861 sends a turn intent for 2000 ms. STM32 latches that direction and
runs a non-blocking local sequence: ramp-brake 360 ms when already moving;
reverse 450 ms; direction-change pause 100 ms; spin 500 ms; then probe forward
at low speed for 250 ms. An `S` command remains an immediate emergency stop in
every phase. Cruise, reverse, turn, and probe use separate reduced PWM targets,
and the target ramp is 250 counts per 20 ms to reduce jerk.

Open `USER/Template.uvprojx` to build. The current project includes
`soft_uart_rx.c`, `autonomous_protocol.c`, and `autonomous_behavior.c`.
