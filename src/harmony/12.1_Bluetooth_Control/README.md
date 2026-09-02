# Hi3861 手机蓝牙遥控桥接

本模块用于 QST先锋号（QST-OH-Multi-Car v1.2）的双芯片遥控链路。手机通过车载蓝牙模块把字符送入Hi3861；Hi3861解析字符并持续向STM32发送双轮速度帧。配对工程为 `src/stm32/7.1_Bluetooth_Control`。

## 链路

```text
手机 --BLE--> 蓝牙模块 --UART1/GPIO1--> Hi3861
Hi3861 --UART2/GPIO11--> STM32 USART1/PA10 --> 电机闭环
```

- 蓝牙UART1：GPIO0 TX、GPIO1 RX，固定`9600-8-N-1`，初始化方式与同型号小车的成功工程一致。
- 车控UART2：GPIO11 TX、GPIO12 RX，`115200-8-N-1`。
- 手机字符：`W`前进、`A`左转、`S`后退、`D`右转、`O`停止；接受小写，忽略空白。
- Hi3861每100ms刷新一次当前命令，避免STM32的2秒失联保护误停车。
- Hi3861优先启动UART2安全停车心跳；即使蓝牙UART1初始化失败，STM32也应收到停止帧并显示白色停止灯。
- 成功基线只使用手机到小车的单向控制，不向手机主动发送初始化、状态或ACK；Iter1以发送`W/A/S/D/O`后小车产生对应动作为准。
- 上电默认停止；非法字符切换为停止。动作会保持到下一条有效命令，因此测试结束必须发送`O`。

## 文件与构建

- `bluetooth_control.c`：双UART初始化、接收线程和周期转发线程。
- `bluetooth_protocol.c/.h`：字符解析、动作映射和`FC ... FD`帧编码。
- `BUILD.gn`：目标`bluetooth_control`。
- `tests/`：可在Windows主机运行的协议测试。

根 `src/harmony/BUILD.gn` 应只启用 `12.1_Bluetooth_Control:bluetooth_control`。远程OpenHarmony全量构建已通过；烧录和手机实测仍由用户完成。
