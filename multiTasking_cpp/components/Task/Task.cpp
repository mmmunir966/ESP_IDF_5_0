#include "Task.hpp"

TaskHandle_t Task::task_handler = NULL;
Task::Task(char *tName)
{
    if (tName == NULL)
    {
        ESP_LOGE(TAG, "Could not create task as name is invalid.");
    }
    else
    {
        taskName = new char[strlen(tName)];
        strcpy(taskName, tName);
        createTask();
    }
}

Task::~Task()
{
    if (taskName)
        delete[] taskName, taskName = NULL;

    deleteTask();
}

void Task::createTask()
{
    xTaskCreate(startTask, "TaskClass", 2048, this, 5, &task_handler);
}

void Task::startTask(void *pvParam)
{
    Task *task = (Task *)pvParam;
    task->runTask(NULL);
}

void Task::runTask(void *pvParam)
{
    while (1)
    {
        ESP_LOGI(TAG, "Printing from %s.", taskName);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Task::deleteTask()
{
    if (task_handler)
        vTaskDelete(task_handler), task_handler = NULL;
}