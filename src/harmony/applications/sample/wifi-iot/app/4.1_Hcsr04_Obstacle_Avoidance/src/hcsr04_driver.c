#include "hcsr04_driver.h"

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "hi_io.h"
#include "hi_time.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"

#define HCSR04_TRIG_GPIO 7U
#define HCSR04_ECHO_GPIO 8U
#define HCSR04_GPIO_FUNC 0U
#define HCSR04_TIMEOUT_US 30000U
#define HCSR04_MIN_CM 2.0F
#define HCSR04_MAX_CM 400.0F
#define HCSR04_SAMPLE_COUNT 3U
#define HCSR04_MAX_ATTEMPTS 5U
#define HCSR04_SAMPLE_GAP_US 60000U

static bool WaitForLevel(WifiIotGpioValue expected, uint32_t timeoutUs,
                         uint32_t *edgeTimeUs)
{
    uint32_t started = hi_get_us();
    WifiIotGpioValue value = WIFI_IOT_GPIO_VALUE0;

    while ((uint32_t)(hi_get_us() - started) <= timeoutUs) {
        if (GpioGetInputVal(HCSR04_ECHO_GPIO, &value) != WIFI_IOT_SUCCESS) {
            return false;
        }
        if (value == expected) {
            if (edgeTimeUs != NULL) {
                *edgeTimeUs = hi_get_us();
            }
            return true;
        }
    }
    return false;
}

static bool ReadOnce(float *distanceCm)
{
    uint32_t risingUs;
    uint32_t fallingUs;
    float distance;

    (void)GpioSetOutputVal(HCSR04_TRIG_GPIO, WIFI_IOT_GPIO_VALUE0);
    hi_udelay(2U);
    (void)GpioSetOutputVal(HCSR04_TRIG_GPIO, WIFI_IOT_GPIO_VALUE1);
    hi_udelay(12U);
    (void)GpioSetOutputVal(HCSR04_TRIG_GPIO, WIFI_IOT_GPIO_VALUE0);

    if (!WaitForLevel(WIFI_IOT_GPIO_VALUE1, HCSR04_TIMEOUT_US, &risingUs) ||
        !WaitForLevel(WIFI_IOT_GPIO_VALUE0, HCSR04_TIMEOUT_US, &fallingUs)) {
        return false;
    }

    distance = (float)(uint32_t)(fallingUs - risingUs) * 0.0343F / 2.0F;
    if ((distance < HCSR04_MIN_CM) || (distance > HCSR04_MAX_CM)) {
        return false;
    }

    *distanceCm = distance;
    return true;
}

bool Hcsr04_Init(void)
{
    if (GpioInit() != WIFI_IOT_SUCCESS) {
        return false;
    }
    if ((hi_io_set_func(HCSR04_TRIG_GPIO, HCSR04_GPIO_FUNC) != WIFI_IOT_SUCCESS) ||
        (hi_io_set_func(HCSR04_ECHO_GPIO, HCSR04_GPIO_FUNC) != WIFI_IOT_SUCCESS) ||
        (GpioSetDir(HCSR04_TRIG_GPIO, WIFI_IOT_GPIO_DIR_OUT) != WIFI_IOT_SUCCESS) ||
        (GpioSetDir(HCSR04_ECHO_GPIO, WIFI_IOT_GPIO_DIR_IN) != WIFI_IOT_SUCCESS)) {
        return false;
    }
    return GpioSetOutputVal(HCSR04_TRIG_GPIO, WIFI_IOT_GPIO_VALUE0) ==
           WIFI_IOT_SUCCESS;
}

bool Hcsr04_ReadMedian(float *distanceCm)
{
    float samples[HCSR04_SAMPLE_COUNT];
    uint32_t validCount = 0U;
    uint32_t attempts;
    uint32_t i;
    uint32_t j;

    if (distanceCm == NULL) {
        return false;
    }

    for (attempts = 0U;
         (attempts < HCSR04_MAX_ATTEMPTS) &&
         (validCount < HCSR04_SAMPLE_COUNT);
         ++attempts) {
        if (ReadOnce(&samples[validCount])) {
            ++validCount;
        }
        if ((attempts + 1U) < HCSR04_MAX_ATTEMPTS) {
            usleep(HCSR04_SAMPLE_GAP_US);
        }
    }

    if (validCount == 0U) {
        return false;
    }

    for (i = 0U; i + 1U < validCount; ++i) {
        for (j = i + 1U; j < validCount; ++j) {
            if (samples[j] < samples[i]) {
                float temporary = samples[i];
                samples[i] = samples[j];
                samples[j] = temporary;
            }
        }
    }

    *distanceCm = samples[validCount / 2U];
    return true;
}
