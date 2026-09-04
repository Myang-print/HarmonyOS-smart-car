# Iter0 最后两组补测

目的：补齐 `drive_log.txt` 中缺少的真正死路样本和合法右支路样本。用户只需手推并原样反馈日志，不需要解释数据。

## 固件与烧录

继续使用已经烧录的 Iter0 红外诊断固件，不需要修改源码。若 Hi3861 中仍是该固件，本次无需重新烧录。

```text
Feature: 17_Iter0_Sensor_Diagnostic:iter0_sensor_diagnostic
```

STM32 不需要烧录。保持车轮悬空或断开电机动力电源。

## TEST-RIGHT-BRANCH

1. 清空 UartAssistant 接收窗口。
2. 将车头放在 Y 字主路、距离交汇区约 10 cm 的位置。
3. 手推小车沿主路进入 Y 字交汇区。
4. 这次明确转入右侧的合法黑胶带分支，并继续沿右支路约 20 cm。
5. 停止并保存全部 `ITER0,SENSOR` 日志。

反馈标题写：

```text
TEST-RIGHT-BRANCH
```

## TEST-TRUE-DEAD-END

1. 清空 UartAssistant 接收窗口。
2. 确认使用的是 `figures/死路.jpg` 对应的单横带死路，不是“干”字型起点/终点。
3. 将车头放在死路主路径、距离单条横向黑带约 10 cm 的位置。
4. 手推小车沿主路径通过这一条横带，并继续到整车越过该标记约 20 cm。
5. 停止并保存全部 `ITER0,SENSOR` 日志。

反馈标题写：

```text
TEST-TRUE-DEAD-END
```

## 反馈

将两组日志追加到 `docs/inbox/drive_log.txt` 或直接发送给开发者：

```text
# TEST-RIGHT-BRANCH
完整连续日志

# TEST-TRUE-DEAD-END
完整连续日志
```

不要将经过“干”字型终点的日志标为死路。出现车轮转动、发热、异味、异常声音、乱码或 `ITER0,INIT_ERROR` 时立即停止并反馈。
