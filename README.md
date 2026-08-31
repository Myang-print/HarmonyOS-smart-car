# HarmonyOS Smart Car 🚗

一个基于 **Hi3861 + STM32F103** 的智能小车学习与实验项目，涵盖传感器采集、串口与 GPIO 通信、电机驱动、编码器测速、PID 控制、灯光效果和自主避障等内容。

- 📡 **Hi3861**：负责超声波、温湿度、光照等传感器数据采集与上层控制逻辑。
- ⚙️ **STM32**：负责电机 PWM、编码器、PID 闭环控制和 WS2812 状态灯。
- 🧪 **分阶段示例**：各目录按实验阶段独立保存，便于学习、编译和调试。

> ⚠️ `4.1_Hcsr04_Obstacle_Avoidance` 与 `41_Autonomous_Obstacle_Avoidance` 为自主避障联调版本，目前仍需继续进行实车通信与运动验证。

## 📁 项目结构

```text
HarmonyOS-smart-car/
├── docs/                         # 学习笔记与开发文档
│   ├── git-sync-workflow.md
│   └── notebooks/                # 分日学习记录
├── materials/                    # 驱动、烧录工具、开发环境与参考资料
├── src/
│   ├── harmony/                  # Hi3861 / OpenHarmony 示例
│   │   └── applications/sample/wifi-iot/app/
│   │       ├── 4.0_Hcsr04_Tick/              # 超声波测距
│   │       ├── 4.1_Hcsr04_Obstacle_Avoidance/ # 避障决策与指令发送
│   │       ├── 7.0_I2c_Ssd1306/              # OLED 显示
│   │       ├── 8.0_Sht20/                    # 温湿度检测
│   │       └── 9.0_Ap3216c/                  # 环境光与接近检测
│   └── stm32/                    # STM32F103 / Keil 工程
│       ├── 4_PWM驱动电机/                    # 电机 PWM 驱动
│       ├── 5_Timer编码器测速/                 # 编码器测速
│       ├── 6_PID电机闭环控制/                 # PID 与灯光控制
│       └── 41_Autonomous_Obstacle_Avoidance/  # 自主避障执行端
└── README.md
```

## 🛠️ 开发环境

- Hi3861：OpenHarmony、Linux/VMware、HiBurn
- STM32：Keil MDK、ST-Link
- 调试：USB 转串口、UartAssist
