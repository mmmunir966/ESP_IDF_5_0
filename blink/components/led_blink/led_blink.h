#ifndef LED_BLINK_H
#define LED_BLINK_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

void blink_led(void);
void configure_led(void);

uint8_t get_led_state();
void toggle_led_state();
#endif