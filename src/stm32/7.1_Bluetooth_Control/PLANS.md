# Iter1 STM32计划

1. 恢复`7_串口通信`的USART1车控接收路径，移除错误的USART3直控路径。
2. 校验帧格式、速度范围、50ms控制周期和2秒失联停车。
3. 运行ARMCC5全量Rebuild并生成当前HEX。
4. 与`12.1_Bluetooth_Control`配对烧录，按`W/A/S/D/O`完成架空实测。
