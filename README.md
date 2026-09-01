# HarmonyOS Smart Car

Hi3861负责传感器、路径/避障决策和上层任务，STM32F103负责串口接收、电机PWM、编码器闭环与WS2812状态灯。仓库按实验阶段保存可独立构建的示例，同时保留两套不同目的的Hi3861—STM32控制链路。

## 系统分工

```text
传感器/路径规划                 实时执行与安全保护
Hi3861  ── UART命令 ──>  STM32F103  ──>  电机驱动/编码器/WS2812
```

| 平台 | 主要职责 | 代码入口 |
| --- | --- | --- |
| Hi3861 / OpenHarmony | GPIO/I2C传感器、RTOS任务、路径与避障决策、发送车控帧 | `src/harmony/` |
| STM32F103 / Keil | 协议解析、电机PWM、编码器PI、失联停车、灯光诊断 | `src/stm32/` |

## 当前两套车控链路

| 用途 | Hi3861 | STM32 | 接线与协议 |
| --- | --- | --- | --- |
| 双轮速度与轨迹控制 | `12.0_UART_Correspondence` | `7_串口通信` | GPIO11→PA10，115200，`FC dir speed dir speed FD` |
| 超声波自主避障 | `4.1_Hcsr04_Obstacle_Avoidance` | `41_Autonomous_Obstacle_Avoidance` | GPIO6→PB8，2400，`A5 5A seq cmd xor` |

两套链路的引脚、波特率和帧格式互不兼容。烧录前必须成对选择Hi3861与STM32工程，并让两块控制板共地。

当前速度控制协议采用6字节帧，方向0为正转、1为反转，速度范围0~250；STM32超过2秒未收到合法帧会停车。详细帧、灯光诊断和V5综合路径见两端模块README。

## 目录

```text
HarmonyOS-smart-car/
├── AGENTS.md                 # Agent修改、提交和README同步规则
├── docs/
│   ├── workflows/            # Git同步与推送流程
│   └── notebooks/            # Linux、远程开发等主题笔记
├── materials/                # 驱动、工具、镜像、资料与参考代码分类
├── src/
│   ├── harmony/              # 可同步到OpenHarmony app/的Hi3861示例
│   └── stm32/                # 独立STM32F103 Keil阶段工程
└── README.md
```

进一步导航：

- [Hi3861示例索引](src/harmony/README.md)
- [STM32工程索引](src/stm32/README.md)
- [开发文档](docs/README.md)
- [资源分类](materials/README.md)
- [Git同步工作流](docs/workflows/git-sync-workflow.md)

## 构建

Hi3861目录是OpenHarmony `app/` 内容快照，不是完整源码树。把 `src/harmony/BUILD.gn` 和目标示例同步到完整源码树的 `applications/sample/wifi-iot/app/`，只启用需要的Feature，再运行对应OpenHarmony版本的构建命令。

STM32在目标工程的 `USER/` 中打开 `Template.uvprojx`，Rebuild后烧录 `OBJ/Template.hex`。构建产物必须来自当前源码，不能复用名称相同但时间较早的HEX。

## 验收原则

- 静态检查只证明文本、协议或算法约束一致。
- OpenHarmony `BUILD SUCCESS` 与Keil零错误只证明对应快照成功构建。
- 烧录、接线、供电、串口波形、轮向、速度和轨迹必须分别实测。
- 所有运动测试先架空车轮；USB-TTL TX和Hi3861 TX不得同时驱动STM32同一RX。
