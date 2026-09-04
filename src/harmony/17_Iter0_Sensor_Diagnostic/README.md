# Iter0 红外传感器诊断固件

本固件只读取 Hi3861 GPIO13（左红外）与 GPIO14（右红外），每 100 ms 输出一行结构化日志。它不初始化车控 UART、不发送运动命令、不驱动电机，用于在实现循迹算法前确定传感器的实际电平和覆盖范围。

## 构建目标

```gn
"17_Iter0_Sensor_Diagnostic:iter0_sensor_diagnostic"
```

本目录中的 `BUILD.gn` 只定义诊断库；复制整个 `src/harmony/` 后，根目录 `src/harmony/BUILD.gn` 会负责启用它。无需单独处理本目录构建文件。

## 用户操作

不需要理解或判断日志。按 `TEST-INSTRUCTIONS.md` 放置小车/胶带，并把每一步的完整 UART 文本原样反馈给开发者；如果无法复制文本，提供清晰截图。

## 日志格式

```text
ITER0,READY,SENSOR_DIAGNOSTIC,GPIO13=LEFT,GPIO14=RIGHT
ITER0,SENSOR,N=1,L=0,R=1
ITER0,SENSOR_READ_ERROR,N=2,L_RC=...,R_RC=...
```

只接受以 `ITER0,` 开头的行作为本固件证据。
