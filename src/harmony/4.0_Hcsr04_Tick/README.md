# HC-SR04 Tick 测距

GPIO7输出Trig脉冲，GPIO8读取Echo，计算并周期打印厘米距离。它用于验证超声波接线和计时逻辑，是 `4.1_Hcsr04_Obstacle_Avoidance` 的传感器基础版本。

Echo通常为5V，接入Hi3861前必须使用分压或电平转换到3.3V。
