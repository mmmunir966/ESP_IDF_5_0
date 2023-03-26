#include "Task.hpp"

char *TAG = "MAIN";

extern "C" void app_main(void)
{
    Task task1("Task 1");
    Task task2("Task 2");
    Task task3(NULL);

    while (1)
    {
        ESP_LOGI(TAG, "Printing from MAIN loop.");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
