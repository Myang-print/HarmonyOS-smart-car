# STM32F103 固件工程

每个一级目录都是相对独立的Keil工程，实际构建入口统一为 `USER/Template.uvprojx`。`CORE/`、`SYSTEM/`、`STM32F10x_FWLib/`提供启动与标准外设库，`QST_HARDWARE/`保存电机、编码器、灯带和控制逻辑，`OBJ/`是构建产物。

| 工程 | 功能与效果 | 关键接口 |
| --- | --- | --- |
| `1_工程模板_跑马灯/` | 最小工程与WS2812跑马灯 | PC13/PC14灯带 |
| `2_串口收发打印/` | USART字符命令切换五种灯效 | USART1 115200 |
| `4_PWM驱动电机/` | 开环PWM演示六种运动并绑定灯效 | TIM4、电机方向GPIO |
| `5_Timer编码器测速/` | 双编码器测速并周期串口输出 | TIM2、TIM3、USART1 |
| `6_PID电机闭环控制/` | 编码器PI、90°转向与闭合路径 | TIM2/3/4、WS2812 |
| `7_串口通信/` | 接收Hi3861双轮速度帧并闭环执行 | PA10/USART1，115200 |
| `41_Autonomous_Obstacle_Avoidance/` | 接收Hi3861离散避障命令并执行状态机 | PB8软件UART，2400 |

## 构建与验收

在目标工程的 `USER/` 中打开 `Template.uvprojx`，Rebuild后烧录 `OBJ/Template.hex`。命令行构建可使用：

```powershell
C:\Keil_v5\UV4\UV4.exe -b .\Template.uvprojx -j0 -o rebuild.log
```

`0 Error(s), 0 Warning(s)`只证明当前源码完成编译链接；轮向、灯光、串口波形、速度和路径角度仍需要下载当前HEX后实车验收。不要根据旧OBJ、MAP或HEX反推当前源码行为。
