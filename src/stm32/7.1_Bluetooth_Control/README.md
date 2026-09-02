# STM32 蓝牙遥控执行端

本工程以`7_串口通信`为基线，只接收Hi3861转发的双轮速度帧，不直接连接手机蓝牙模块。配对的上位固件为`src/harmony/12.1_Bluetooth_Control`。

## 接口和行为

- Hi3861 GPIO11/UART2 TX → STM32 PA10/USART1 RX，共地，`115200-8-N-1`。
- 帧格式：`FC 左方向 左速度 右方向 右速度 FD`；方向0正转、1反转，速度0~250。
- 主循环每50ms运行一次电机闭环；超过2秒没有合法帧自动停车。
- 绿灯前进、红灯后退、蓝灯左转、橙灯右转；等待、非法帧和失联使用诊断灯效。
- USART3/PB10/PB11不参与本功能，避免再次绕过Hi3861。

## 构建

入口为`USER/Template.uvprojx`，输出为`OBJ/Bluetooth_Control.hex`。ARMCC5全量Rebuild已通过，结果为0 Error、0 Warning；烧录和架空动作仍需用户验证。
