# Iter1 低速直线循迹诊断日志记录 01

## 来源

- 原始日志：`docs/inbox/Iter1_Line_Follow_Diagnostic_log.txt`
- 测试操作单：`tests/hardware/ITER1-line-follow-TEST.md`
- Hi3861 固件：`20_Iter1_Line_Follow_Diagnostic:iter1_line_follow_diagnostic`
- STM32 固件：已验证的 `7.1_Bluetooth_Control`
- STM32 日志：未提供

## Hi3861 启动确认

```text
ITER1,READY,LINE_FOLLOW_DIAGNOSTIC,UART2_GPIO11_TX,115200
ITER1,SAFETY,LOW_SPEED_ONLY,11_STOP,STOP_ON_UNCERTAIN_OR_ERROR
```

确认诊断固件成功启动，并声明低速及不确定状态停车策略。

## 传感器与程序状态

日志包含：

- 初始 `L=0,R=0,STATE=STOP`；
- `L=0,R=1` 连续出现后进入 `STATE=RIGHT_LINE`；
- `L=1,R=1` 连续出现后进入 `STATE=UNCERTAIN_STOP`；
- 后续再次出现 `L=0,R=1` 连续状态并进入 `RIGHT_LINE`；
- 多次 `L=0,R=0` 保持 `UNCERTAIN_STOP`。

日志序号从 N=1 开始连续递增到 N=119，没有观察到序号缺失。

## 用户肉眼反馈

```text
车轮是否转动：转动
放白色区域时是否停止：停止
左探头放黑胶带时是否出现动作：出现动作
右探头放黑胶带时是否出现动作：出现动作
最后两个探头都在白色区域时是否停止：是
是否发热、异味或异常声音：无
```

## 已确认

1. Hi3861 能持续读取 GPIO13/14；
2. `L=0,R=0` 已修正为 `CENTERED_FORWARD`，即两个探头在黑线两侧白色包边时低速直行；
3. `L=1,R=0` 进入 `LEFT_LINE` 差速修正；
4. `L=0,R=1` 进入 `RIGHT_LINE` 差速修正；
5. `L=1,R=1` 连续确认后进入 `UNCERTAIN_STOP` 停车；
6. 传感器状态变化能触发车控帧状态变化；
7. 左探头和右探头覆盖黑胶带时均能产生电机动作；
8. 未发现发热、异味或异常声音。

- `L=1,R=1` 是一个短暂非法几何状态，旧版会立即停车；本轮已证明该策略会导致车辆无法继续沿路径移动。

- 车轮落地时能否沿普通黑胶带稳定前进；
- 左右差速修正的实际方向是否正确；
- `L=1,R=0` 的完整日志和动作是否符合预期；
- 传感器从 `00` 恢复到 `10/01` 时是否会持续跟线；
- Y 字路口、干字型标记和死路处理。

## 结论

本轮通过“低速、架空、传感器触发差速命令”的诊断，不等同于普通直线自动循迹通过。下一轮应只测试普通直线黑胶带落地运行，仍保持低速、可断电和不进入任何路口。
