# Iter1 电机链路测试日志记录 01

## 测试来源

- 用户反馈：UartAssistant Hi3861 日志
- 测试固件：`18_Iter1_Motor_Diagnostic:iter1_motor_diagnostic`
- 日志时间：2026-09-05 10:46:46.477 至 10:47:00.824
- STM32 日志：本次未提供

## Hi3861 端已确认

### 初始化

```text
ITER1,READY,MOTOR_DIAGNOSTIC,UART2_GPIO11_TX,115200,FRAME=FC_DIR_SPEED_DIR_SPEED_FD
ITER1,SAFETY,WHEELS_MUST_BE_SUSPENDED,STOP_ON_UNEXPECTED_MOTION
```

确认：

- Iter1 电机诊断固件成功启动；
- 使用 UART2 GPIO11 TX；
- 波特率 115200；
- 发送格式为 `FC DIR SPEED DIR SPEED FD`；
- 安全提示已输出。

### 动作调度

| INDEX | ACTION | 左轮 | 右轮 | 时长 | 发送帧数 |
| ---: | --- | --- | --- | ---: | ---: |
| 0 | STOP_BEFORE | 0 | 0 | 1200 ms | 12 |
| 1 | FORWARD | 40 | 40 | 1200 ms | 12 |
| 2 | STOP_AFTER_FORWARD | 0 | 0 | 1200 ms | 12 |
| 3 | REVERSE | -40 | -40 | 1200 ms | 12 |
| 4 | STOP_AFTER_REVERSE | 0 | 0 | 1200 ms | 12 |
| 5 | TURN_LEFT | -40 / +40 | 1200 ms | 12 |
| 6 | STOP_AFTER_LEFT | 0 | 0 | 1200 ms | 12 |
| 7 | TURN_RIGHT | +40 / -40 | 1200 ms | 12 |
| 8 | STOP_AFTER_RIGHT | 0 | 0 | 1200 ms | 12 |
| 9 | FINAL_STOP | 0 | 0 | 3000 ms | 30 |

所有 10 个动作均输出 `ACTION_START` 和 `ACTION_END`，没有出现 `ITER1,MOTOR_WRITE_ERROR`。

## 当前证据结论

### 已确认

- Hi3861 诊断程序成功运行；
- UART2/GPIO11 发送端初始化成功；
- 10 个动作按顺序执行；
- 发送端每 100 ms 刷新一次；
- 所有预定停车阶段均发送 12 帧停车帧；
- 最终停车阶段发送 30 帧停车帧；
- 本次 Hi3861 日志中没有发送写入错误。

### 尚未确认

- STM32 是否成功烧录正确的 `Bluetooth_Control.hex`；
- GPIO11 是否实际连接到 STM32 PA10；
- 两板是否共地；
- STM32 是否收到并通过 6 字节帧校验；
- STM32 电机是否执行前进、后退、左转、右转；
- 每个停车阶段电机是否停止；
- STM32 2 秒失联停车是否生效。

本记录不把 Hi3861 发送成功等同于整条链路成功。
