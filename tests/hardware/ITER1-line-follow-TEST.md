# Iter1 直线循迹：全前进边缘跟随测试

上一版原地扫描恢复方案已失败，本版取消所有反向和原地旋转动作。

## 烧录

只重新构建并烧录 Hi3861。STM32 保持当前已验证固件。

复制：

```text
D:\_GitHub\30_MyProjects\HarmonyOS-smart-car\src\harmony\
```

到虚拟机：

```text
applications/sample/wifi-iot/app/
```

确认 `BUILD.gn` 只启用：

```text
20_Iter1_Line_Follow_Diagnostic:iter1_line_follow_diagnostic
```

构建：

```bash
python build.py wifiiot
```

## 本轮唯一目标

验证任何传感器状态下车辆都不会反向移动、原地旋转或因 `11` 停车：

```text
00 -> 100 / 100 正向直行
10 -> 45 / 100 正向左修正
01 -> 100 / 45 正向右修正
11 -> 保持最近一次正向命令；无历史时 100 / 100 正向直行
```

只有传感器读取失败才允许发送停车帧。

## 安全测试

1. 第一次必须架空车轮。
2. 准备立即断开电机动力电源。
3. 先观察 5 秒普通白色区域。
4. 依次让左探头、右探头和两个探头进入黑胶带状态。
5. 任何反向运动、原地转圈、持续异常运动、发热、异味或异常声音，立即断电。
6. 架空测试通过后，落地运行普通直线不超过 3 秒。

## 预期启动日志

```text
ITER1,READY,LINE_FOLLOW_FSM,UART2_GPIO11_TX,115200
ITER1,SAFETY,FORWARD_ONLY_EDGE_TRACKING,11_KEEP_LAST_FORWARD,NO_REVERSE_NO_SPIN,STOP_ON_SENSOR_ERROR
```

运行日志中不应出现：

```text
CMD=SPIN_LEFT
CMD=SPIN_RIGHT
```

## 返回内容

复制从 `ITER1,READY` 开始的完整日志，并填写：

```text
架空白色区域时车轮是否持续正向转动：
左探头黑胶带状态是否仍为正向左修正：
右探头黑胶带状态是否仍为正向右修正：
两个探头同时为 1 时是否保持正向运动：
是否出现反向运动：
是否出现原地转圈：
是否因 11 停车：
落地 3 秒内是否仍需要人为扰动：
是否发热、异味或异常声音：
```
