# 自主避障执行端

接收 `src/harmony/4.1_Hcsr04_Obstacle_Avoidance` 发出的离散动作命令，在STM32本地执行停车、倒车、转向和低速探测状态机。

## 链路与协议

- Hi3861 GPIO6/UART1 TX → STM32 PB8软件UART RX，两板共地。
- 2400-8-N-1，帧为 `A5 5A sequence command xor`。
- 命令为 `F`前进、`S`停止、`L`左转、`R`右转。
- 每帧重复发送；STM32校验命令、序号和XOR，1秒无合法帧自动停车。

PB8使用上拉输入，EXTI检测起始位，TIM3完成位采样，TIM4继续负责电机PWM。该协议与 `7_串口通信` 的PA10/115200六字节速度协议不兼容。

## 行为与灯光

转向意图触发非阻塞序列：制动、倒车、方向切换暂停、原地旋转、低速向前探测；任意阶段收到 `S` 都立即停车。红闪表示未建立链路或超时，暗橙表示停车，蓝/白表示前进或探测，蓝色渐变表示左转，红色渐变表示右转。

构建入口为 `USER/Template.uvprojx`，关键源码是 `soft_uart_rx.c`、`autonomous_protocol.c` 和 `autonomous_behavior.c`。
