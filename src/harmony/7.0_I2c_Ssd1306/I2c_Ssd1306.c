/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hal_bsp_ssd1306.h"


void Task1(void)
{
    uint8_t displayBuff[20] = {0};
    uint8_t hour = 16, min = 0, sec = 0;

    SSD1306_Init();    // OLED 显示屏初始化
    SSD1306_CLS();     // 清屏

    SSD1306_ShowStr(0, 0, (uint8_t *)"    QST CAR  ", 16);
    SSD1306_ShowStr(0, 3, (uint8_t *)"2025:10:08", 16);

    while (1)
    {
        sec++;

        if (sec > 59)
        {
            sec = 0;
            min++;
        }

        if (min > 59)
        {
            min = 0;
            hour++;
        }

        if (hour > 23)
        {
            hour = 0;
        }

        // 清除 displayBuff 中字符串
        memset(displayBuff, 0, sizeof(displayBuff));

        sprintf((char *)displayBuff, "%02d:%02d:%02d", hour, min, sec);

        // 写入 OLED 显示
        SSD1306_ShowStr(0, 2, (uint8_t *)displayBuff, 16);

        sleep(1);    // 1 s
    }
}


static void i2c_sdd1306_demo(void)
{
    osThreadAttr_t options;

    options.name = "thread_1";
    options.attr_bits = 0;
    options.cb_mem = NULL;
    options.cb_size = 0;
    options.stack_mem = NULL;
    options.stack_size = 1024;
    options.priority = osPriorityNormal;

    osThreadId_t Task1_ID;

    Task1_ID = osThreadNew((osThreadFunc_t)Task1, NULL, &options);

    if (Task1_ID != NULL)
    {
        printf("ID = %d, Create Task1_ID is OK!", Task1_ID);
    }
}


APP_FEATURE_INIT(i2c_sdd1306_demo);