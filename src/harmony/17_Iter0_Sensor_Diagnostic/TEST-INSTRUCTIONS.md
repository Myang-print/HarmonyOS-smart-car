# Iter0 第一次硬件取证操作单

你不需要解释任何数据，只需执行放置动作并原样反馈程序打印内容。

## 要烧录什么

只烧录 Hi3861：

```text
src/harmony/17_Iter0_Sensor_Diagnostic
构建目标：17_Iter0_Sensor_Diagnostic:iter0_sensor_diagnostic
```

本轮不要重新烧录 STM32。诊断固件不会发送电机命令；为进一步保证安全，请让车轮悬空或断开电机动力电源。

## 烧录前构建选择

复制整个 `src/harmony/` 到虚拟机后，仓库中的 `src/harmony/BUILD.gn` 已经为本轮诊断准备好。不要修改构建文件，直接在你平时执行构建的目录运行：

```text
python build.py wifiiot
```

本轮不需要使用 `APP_BUILD.gn`；它只是保留的独立参考文件，不是操作必需文件。

预期启用的唯一 Feature 是：

```gn
"17_Iter0_Sensor_Diagnostic:iter0_sensor_diagnostic",
```

其他业务 Feature 全部注释，尤其不要同时启用 `12.1_Bluetooth_Control`、`12.0_UART_Correspondence` 或 `2.1_TableGuard`。

## UartAssistant

使用你平时查看 Hi3861 `printf` 日志的串口和原有串口参数。若看不到可读文本，不要自行猜参数，只把界面截图发回。

看到下列开头即表示固件正确：

```text
ITER0,READY,SENSOR_DIAGNOSTIC,GPIO13=LEFT,GPIO14=RIGHT
```

## 依次执行并反馈

每一步保持约 3 秒。不要挑选日志，也不要告诉我“高/低”或“正常/异常”；直接复制该步骤期间所有 `ITER0,` 行。每组日志前写步骤号即可。

1. `STEP-0`：红外探头保持悬空，不放任何胶带或地面到探头下。
2. `STEP-1`：把左右两个探头都放在普通白色地面/白底上。
3. `STEP-2`：把左右两个探头都放在黑胶带正上方。
4. `STEP-3`：只让左探头位于黑胶带上，右探头位于白底上。
5. `STEP-4`：左探头位于白底上，只让右探头位于黑胶带上。
6. `STEP-5`：按小车实际行驶方向，把黑胶带置于两探头之间；两探头都不要直接压在黑胶带上。
7. `STEP-6`：如果赛道的“白胶带围边”和普通白底肉眼材质/颜色不同，把两个探头都放在白色围边胶带上；若完全相同，仍执行一次。

建议反馈格式：

```text
STEP-0
ITER0,...
ITER0,...

STEP-1
ITER0,...
ITER0,...
```

如果日志很多，允许每一步只截取连续 20 行，但必须包含步骤开始后稳定放置期间的连续行，不要人工筛选。

## 立即停止并反馈的情况

- 车轮发生运动；
- 没有任何 `ITER0,READY`；
- 只有乱码；
- 出现 `ITER0,INIT_ERROR`；
- 持续出现 `ITER0,SENSOR_READ_ERROR`。

遇到上述情况时不要自行修改代码或接线，只发送完整文本或截图。
