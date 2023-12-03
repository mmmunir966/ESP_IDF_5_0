#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdio.h>
#include <string.h>
#include "esp_mac.h"
#include "sdkconfig.h"
#include "sys/socket.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "esp_log.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "cJSON.h"

#define DEFAULT_AP_SSID "ESP32_WiFI_MANAGER"
#define DEFAULT_AP_PASSWORD "Password"
#define DEFAULT_HOSTNAME "esp32"
#define DEFAULT_AP_IP "192.168.4.1"
#define DEFAULT_AP_GATEWAY "192.168.4.1"
#define DEFAULT_AP_NETMASK "255.255.255.0"
#define DEFAULT_AP_CHANNEL 1
#define DEFAULT_AP_SSID_HIDDEN 0
#define DEFAULT_AP_MAX_CONNECTIONS 4
#define DEFAULT_AP_BEACON_INTERVAL 100 // Should be multiple of 100.

void start_wifi_manager();

void stop_wifi_manager();
/**
 * @brief It copies the ap_rcords scanned.
 * @param scannedAPList list where the ap-records are copied.
 * @return returns the number of ap scanned.
 * @NOTE: scannedAPList should deleted using `delete[]` command. after calling this function and performing necessary processing.
 */
// uint16_t get_available_networks_list(wifi_ap_record_t **scannedAPList);
char *get_available_networks_list();

/**
 * @brief Check if the device is connected to the wifi.
 *
 * @return true: If the device is connected to the wifi.
 *         false: If the device is not connected to the wifi.
 */
bool is_wifi_connected();

/**
 * @brief Update the wifi station mode credentials.
 * @param config wifi configuration settings for sta mode.
 */
void update_sta_config(wifi_config_t config);

#endif /* WIFI_MANAGER_H*/
