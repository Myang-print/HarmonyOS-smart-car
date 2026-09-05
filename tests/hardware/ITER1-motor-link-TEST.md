# Iter1 电机链路烧录与反馈

## 1. 需要烧录的固件

### Hi3861

复制整个：

```text
src/harmony/
```

到虚拟机中原来的：

```text
applications/sample/wifi-iot/app/
```

本次 `src/harmony/BUILD.gn` 已只启用：

```text
18_Iter1_Motor_Diagnostic:iter1_motor_diagnostic
```

在虚拟机中按原流程执行：

```text
python build.py wifiiot
```

构建并烧录 Hi3861。

### STM32

烧录当前 STM32 蓝牙控制执行端工程生成的 HEX：

```text
src/stm32/7.1_Bluetooth_Control/OBJ/Bluetooth_Control.hex
```

如果你无法确认该 HEX 是否由当前源码重新生成，不要自行选择其他 HEX；直接告诉我。STM32 工程入口为：

```text
src/stm32/7.1_Bluetooth_Control/USER/Template.uvprojx
```

## 2. 接线

确认以下连接；不需要你判断电平：

```text
Hi3861 GPIO11 / UART2_TX -> STM32 PA10 / USART1_RX
Hi3861 与 STM32 共地
```

不要把 USB-TTL TX 和 Hi3861 GPIO11 同时接到 STM32 PA10。

## 3. 安全

- 车轮必须悬空，或断开电机动力电源后先验证日志。
- 上电后程序会自动动作；准备好立即断电。
- 车轮意外转动、无法停止、发热、异味或异常声音时，立即断开电机动力。

## 4. UartAssistant

打开你平时查看 Hi3861 日志的串口。无需修改代码。

应看到：

```text
ITER1,READY,...
ITER1,ACTION_START,...
ITER1,ACTION_END,...
```

Hi3861 日志不会显示 STM32 的接收结果；STM32 的 `REMOTE,...` 日志需要使用 STM32 的串口输出连接查看。如果只有一个 UartAssistant 通道，先反馈 Hi3861 日志即可，并说明“没有 STM32 日志”。

## 5. 反馈内容

程序会依次执行：

```text
停车 -> 前进 -> 停车 -> 后退 -> 停车 -> 左转 -> 停车 -> 右转 -> 停车
```

每个动作约 1.2 秒，速度值为 40，动作之间停车约 1.2 秒。请反馈：

```text
1. Hi3861 UartAssistant 中从 ITER1,READY 开始的完整日志；
2. STM32 串口日志（如果能看到），完整复制；
3. 只补充肉眼现象：车轮是否转动、是否出现前进/后退/左右转、每次停车后是否停止。
```

不需要判断方向对不对，只需描述看到的现象。若看不清方向，写“无法判断方向”。
