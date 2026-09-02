# Hi3861 / OpenHarmony 示例

这里直接保存原 OpenHarmony `applications/sample/wifi-iot/app/` 下的内容，去掉了与本仓库无关的外层源码树。使用时把 `BUILD.gn` 和所需示例目录同步到完整 OpenHarmony 源码树的 `applications/sample/wifi-iot/app/`，再执行对应版本的 `python3 build.py wifiiot`。

## 示例索引

| 目录 | 功能与效果 | 主要接口 |
| --- | --- | --- |
| `1.0_Hello_World/` | LiteOS 多线程与串口打印入门 | CMSIS-RTOS2、UART 日志 |
| `2.0_TCRT_Timer/` | TCRT 定时器实验残留快照，目前文件不完整 | BUILD 引用缺失的 `TCRT.c` |
| `2.1_TableGuard/` | 双红外桌面防跌落，遇边缘后退并转向 | GPIO13/14、UART2 |
| `3.0_SG90_Mutex/` | 多线程与互斥锁控制 SG90 舵机 | GPIO2 软件脉冲 |
| `4.0_Hcsr04_Tick/` | HC-SR04 超声波测距 | GPIO7 Trig、GPIO8 Echo |
| `4.1_Hcsr04_Obstacle_Avoidance/` | 测距决策并向 STM32 发送避障命令 | GPIO6/UART1，2400 baud |
| `7.0_I2c_Ssd1306/` | SSD1306 OLED 时钟显示 | I2C0，GPIO9/10 |
| `8.0_Sht20/` | SHT20 温湿度采集、OLED 显示和任务间传递 | I2C0，GPIO9/10 |
| `9.0_Ap3216c/` | 红外、环境光、接近三合一传感器 | I2C0，GPIO9/10 |
| `12.0_UART_Correspondence/` | 规划综合轨迹并控制 STM32 双轮速度 | GPIO11/UART2，115200 baud |
| `12.1_Bluetooth_Control/` | 接收手机蓝牙字符并转发双轮速度帧 | GPIO0/1 UART1 9600；GPIO11/12 UART2 115200 |
| `demolink/` | OpenHarmony Demo SDK 链接示例 | `SYS_RUN` |
| `iothardware/` | GPIO9 LED 闪烁示例 | GPIO9 |
| `samgr/` | SAMGR 服务、特性、消息与任务示例 | 系统服务框架 |
| `startup/` | 空的启动 source set 占位 | GN 构建 |
| `_archive/` | 历史备份，不作为当前构建输入 | 只读参考 |

## 构建选择

根 `BUILD.gn` 的 `features` 决定实际编入固件的应用。通常一次只启用一个会占用相同 GPIO/UART 的业务示例。当前仅启用 `12.1_Bluetooth_Control`，避免其他任务竞争UART1、UART2或小车控制权。

## 两套车控协议

- `12.0_UART_Correspondence` 配对 `src/stm32/7_串口通信`：GPIO11 → PA10，115200-8-N-1，6字节双轮速度帧。
- `12.1_Bluetooth_Control` 配对 `src/stm32/7.1_Bluetooth_Control`：手机字符经GPIO0/1 UART1进入Hi3861，再由GPIO11 UART2发送同一6字节双轮速度帧。
- `4.1_Hcsr04_Obstacle_Avoidance` 配对 `src/stm32/41_Autonomous_Obstacle_Avoidance`：GPIO6 → PB8，2400-8-N-1，5字节带序号和 XOR 的动作帧。

两套协议的引脚、波特率和帧格式不兼容，不能交叉烧录或混接。
