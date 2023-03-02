#include "led_blink.h"

static const char *TAG = "example";

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", get_led_state() == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        toggle_led_state();
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
