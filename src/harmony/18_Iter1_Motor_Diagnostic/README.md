# Iter1 电机串口链路诊断

本固件自动通过 Hi3861 UART2/GPIO11 向 STM32 发送低速双轮速度帧，验证主车控链路。动作完成后自动发送停车帧。

构建目标：

```text
18_Iter1_Motor_Diagnostic:iter1_motor_diagnostic
```

当前根 `src/harmony/BUILD.gn` 应只启用该 Feature。完整步骤见 `TEST-INSTRUCTIONS.md`。
