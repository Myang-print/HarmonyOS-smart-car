# Iter1 失联停车测试日志记录 01

## 来源

- 原始日志：`docs/inbox/Iter1_log_1.txt`
- 测试操作单：`tests/hardware/ITER1-failsafe-TEST.md`
- Hi3861 固件：`19_Iter1_Failsafe_Diagnostic:iter1_failsafe_diagnostic`
- STM32 固件：上一轮 `7.1_Bluetooth_Control`
- STM32 日志：未提供

## Hi3861 日志关键结果

```text
ITER1,FAILSAFE,FORWARD_ONCE,WRITTEN=6
ITER1,FAILSAFE,SILENCE_NOW,WAIT_4_SECONDS
ITER1,FAILSAFE,OBSERVATION_WINDOW_END
```

- Hi3861 成功发送一次 6 字节前进帧，`WRITTEN=6`；
- 随后进入静默窗口；
- 观察窗口约 4 秒后正常结束；
- 未出现 `ITER1,FAILSAFE,INIT_ERROR` 或任务创建错误。

## 用户肉眼反馈

```text
车轮在收到一次前进帧后是否转动：是
约2秒后是否自动停止：是
最终是否保持停止：是
是否发热：无
是否有异味：无
是否有异常声音：无
```

## 结论

已实车确认：

1. Hi3861 UART2/GPIO11 能向 STM32 发送单帧速度命令；
2. STM32 收到单帧前进命令后会驱动车轮；
3. 停止继续发送后，STM32 约 2 秒内自动停车；
4. 停车后保持停止；
5. 本轮未观察到发热、异味或异常声音。

该结果满足 STM32 失联停车安全门禁。由于没有 STM32 串口日志，不能确认日志中的 `REMOTE,TIMEOUT,STOP` 文本是否输出，但肉眼结果已经确认其运动效果。
