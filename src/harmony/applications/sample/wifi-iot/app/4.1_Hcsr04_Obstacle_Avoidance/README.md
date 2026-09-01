# Hi3861 autonomous obstacle avoidance

The Hi3861 measures distance and sends a framed command to the STM32 through
the existing GPIO6-to-PB8 wire. GPIO6 is now the hardware UART1 TX output at
2400 baud, 8 data bits, no parity and 1 stop bit.

## Wiring

- HC-SR04 Trig: Hi3861 GPIO7
- HC-SR04 Echo: Hi3861 GPIO8 through a 5 V to 3.3 V divider
- Hi3861 GPIO6 (UART1 TX) -> STM32 PB8 (software UART RX)
- Hi3861, STM32, and motor-driver logic grounds: common GND
- The old GPIO5 -> PB9 wire may remain connected but is not used by the protocol

Every transmission contains three copies of a five-byte frame:
`A5 5A sequence command xor`. The command byte is `F`, `S`, `L`, or `R`, and
the XOR byte covers the two sync bytes, sequence, and command. Repetition lets
the STM32 recover if a WS2812 refresh overlaps one serial byte.

## Behavior

- Obstacle threshold: 20 cm
- Clear threshold: 25 cm
- Stop before turning: 500 ms
- Turn-intent hold: 2000 ms
- First valid clear measurement: forward
- One failed median after valid data: reuse the last distance
- Two consecutive failed medians: stop

The measurement loop takes less than the STM32's 1000 ms link timeout even in
the five-attempt invalid-distance path. UART0 on GPIO3 remains the diagnostic
log output; UART1 on GPIO6 is dedicated to the one-way car-control frames.
