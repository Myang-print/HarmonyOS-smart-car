# ITER0 红外传感器原始记录 01

- 来源：UartAssistant 截图
- 记录状态：已确认 GPIO13/14 输出连续且电平语义已由用户结合实物确认；截图对应步骤未知
- 烧录固件：`17_Iter0_Sensor_Diagnostic:iter0_sensor_diagnostic`（依据日志前缀）
- STM32：未确认；诊断固件设计上不依赖 STM32
- 说明：以下内容按截图可见文本转录。电平物理语义由用户结合实物另行确认：`0=白色道路包边`，`1=黑胶带或越过白色包边`。

```text
[2026-09-04 18:29:42.685]# RECV ASCII>
ITER0,SENSOR,N=358,L=1,R=0

[2026-09-04 18:29:42.782]# RECV ASCII>
ITER0,SENSOR,N=359,L=1,R=0

[2026-09-04 18:29:42.875]# RECV ASCII>
ITER0,SENSOR,N=360,L=1,R=0

[2026-09-04 18:29:42.979]# RECV ASCII>
ITER0,SENSOR,N=361,L=1,R=0

[2026-09-04 18:29:43.084]# RECV ASCII>
ITER0,SENSOR,N=362,L=1,R=0

[2026-09-04 18:29:43.171]# RECV ASCII>
ITER0,SENSOR,N=363,L=1,R=0

[2026-09-04 18:29:43.278]# RECV ASCII>
ITER0,SENSOR,N=364,L=1,R=1

[2026-09-04 18:29:43.382]# RECV ASCII>
ITER0,SENSOR,N=365,L=1,R=1

[2026-09-04 18:29:43.474]# RECV ASCII>
ITER0,SENSOR,N=366,L=1,R=1

[2026-09-04 18:29:43.581]# RECV ASCII>
ITER0,SENSOR,N=367,L=1,R=1

[2026-09-04 18:29:43.682]# RECV ASCII>
ITER0,SENSOR,N=368,L=1,R=1

[2026-09-04 18:29:43.787]# RECV ASCII>
ITER0,SENSOR,N=369,L=1,R=1

[2026-09-04 18:29:43.879]# RECV ASCII>
ITER0,SENSOR,N=370,L=1,R=1

[2026-09-04 18:29:43.983]# RECV ASCII>
ITER0,SENSOR,N=371,L=1,R=1

[2026-09-04 18:29:44.071]# RECV ASCII>
ITER0,SENSOR,N=372,L=1,R=1
```

## 机器可核对摘要

- 可见记录数：15
- `N`：358..372，连续递增，无缺号
- `L`：全程为 1
- `R`：N=358..363 为 0；N=364..372 为 1
- 截图时间范围：2026-09-04 18:29:42.685 至 18:29:44.071
- 截图覆盖时间：约 1.386 秒

## 待补充元数据

- 该截图对应 `TEST-INSTRUCTIONS.md` 的哪个 STEP；
- 截图期间两个探头分别放在什么位置；
- 是否连续执行过多个 STEP 后未清空 UartAssistant。

在补齐上述元数据前，本记录只能证明 GPIO13/14 能输出连续采样结果和数值变化，不能用于确定黑胶带/白底对应电平。
