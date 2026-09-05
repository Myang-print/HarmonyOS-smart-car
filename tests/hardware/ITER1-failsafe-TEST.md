# Iter1 失联停车测试

## 烧录

复制整个 `src/harmony/` 到虚拟机原来的 `applications/sample/wifi-iot/app/`，直接执行：

```text
python build.py wifiiot
```

本次根 `src/harmony/BUILD.gn` 已只启用：

```text
19_Iter1_Failsafe_Diagnostic:iter1_failsafe_diagnostic
```

只烧录 Hi3861。STM32 保持上一轮已验证的 `7.1_Bluetooth_Control` 固件，不要重新烧录。

## 安全

- 车轮悬空，或断开电机动力电源后再开始。
- 确认 Hi3861 日志出现 `ITER1,FAILSAFE,FORWARD_ONCE` 后，程序不会继续发送车控帧。
- 如果车轮在约 2 秒后仍明显转动，立即断电。

## 反馈

复制从 `ITER1,FAILSAFE` 开始的全部 Hi3861 日志，并填写：

```text
Hi3861 日志：

STM32 日志：没有/粘贴日志

车轮在收到一次前进帧后是否转动：是/否/不确定
约2秒后是否自动停止：是/否/不确定
最终是否保持停止：是/否/不确定
是否发热、异味或异常声音：原样描述
```
