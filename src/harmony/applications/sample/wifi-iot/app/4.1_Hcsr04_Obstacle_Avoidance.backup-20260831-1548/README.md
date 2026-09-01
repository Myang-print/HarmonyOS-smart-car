# Hi3861 autonomous obstacle avoidance

The Hi3861 measures distance and sends one of four motor modes to STM32 over a
two-wire GPIO link. UART is intentionally not used between chips.

## Wiring

- HC-SR04 Trig: Hi3861 GPIO7
- HC-SR04 Echo: Hi3861 GPIO8 through a 5 V to 3.3 V divider
- Hi3861 GPIO6 MODE0 -> STM32 PB8
- Hi3861 GPIO5 MODE1 -> STM32 PB9
- Hi3861, STM32, motor-driver logic: common GND

Mode encoding is `00=stop`, `01=forward`, `10=left`, `11=right`. The STM32
requires two consecutive identical samples before changing state. Both inputs
are pulled down, so disconnected wires resolve to the safe `00=stop` state.

## Behavior

- Obstacle threshold: 20 cm
- Clear threshold: 25 cm
- Stop before turning: 500 ms
- Spin duration: 300 ms
- Invalid or stale distance: stop

The source file retains its historical `car_uart` name, but it now contains
only GPIO output code and does not initialize a UART peripheral.
