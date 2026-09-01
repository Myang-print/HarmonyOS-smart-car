# Hello World

演示 CMSIS-RTOS2 任务创建与并发打印。两个线程周期输出 `Hello World` 和 `Hello QST`，用于确认 OpenHarmony 应用注册、调度器和调试串口正常。

构建目标为 `1.0_Hello_World:hello_world`，入口由 `APP_FEATURE_INIT(Hello_World)` 注册。
