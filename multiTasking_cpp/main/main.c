#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
