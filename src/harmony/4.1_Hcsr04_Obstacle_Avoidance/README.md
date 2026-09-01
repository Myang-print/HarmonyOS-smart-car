# HC-SR04 自主避障发送端

Hi3861通过GPIO7/8测量距离，并把避障决策发送给 `src/stm32/41_Autonomous_Obstacle_Avoidance`。障碍阈值20cm、清除阈值25cm；连续测距失败会发送停车命令。

## 接口

- HC-SR04：GPIO7 Trig，GPIO8 Echo（Echo需5V转3.3V）。
- 车控链路：GPIO6/UART1 TX → STM32 PB8软件UART RX，共地。
- 串口：2400-8-N-1。
- 帧：`A5 5A sequence command xor`，每次重复3帧。
- 命令：`F`前进、`S`停止、`L`左转、`R`右转。

该协议与UART2六字节速度协议不兼容。源码包含驱动、控制器和主机侧单元测试，构建目标为 `4.1_Hcsr04_Obstacle_Avoidance:Hcsr04Obstacle`。
