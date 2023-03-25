#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "MAIN";

void task1()
{
    while (1)
    {
        ESP_LOGI("TASK1", "Printing from task1.");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task2()
{
    while (1)
    {
        ESP_LOGI("TASK2", "Printing from task2.");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    TaskHandle_t task1_handler = NULL;
    TaskHandle_t task2_handler = NULL;

    xTaskCreate(task1, "task1", 2*1024, NULL, 5, task1_handler);
    xTaskCreate(task2, "task2", 2*1024, NULL, 5, task2_handler);
    while (1)
    {
        ESP_LOGI(TAG, "Printing from MAIN loop.");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
