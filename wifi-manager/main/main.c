#include "web_server.h"
#include "nvs.h"
#include "nvs_flash.h"

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    start_wifi_manager();
    start_web_server();

    size_t free_heap_size = 0;
    while (1)
    {
        free_heap_size = esp_get_free_heap_size();
        ESP_LOGI("MAIN", "Free Heap Size: %u bytes", free_heap_size);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
