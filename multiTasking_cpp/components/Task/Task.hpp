#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "string.h"

class Task
{
    public:
    Task(char* taskName);
    
    ~Task();

    private:
    void createTask();
    static void startTask(void *pvParam);
    void runTask(void *pvParam);
    void deleteTask();

    static constexpr char *TAG = "TaskClass";
    char *taskName = NULL;
    static TaskHandle_t task_handler;
};
