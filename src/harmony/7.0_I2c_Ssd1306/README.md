# SSD1306 OLED

通过I2C0驱动SSD1306，显示标题、日期和软件递增时钟。GPIO10为SDA，GPIO9为SCL；当前示例I2C速率为100kHz。

`include/`保存显示接口与字模，`src/`保存底层驱动，`I2c_Ssd1306.c`是应用入口。
