#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdio.h>
#include <string.h>
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

/**
 * @brief start_wifi_manager
 * Start the wifi manager
 */
void start_wifi_manager();

/**
 * @brief stop_wifi_manager
 * Stop the wifi manager
 */
void stop_wifi_manager();

/**
 * @brief get_available_networks_list
 * returns the list of networks in json format. Contains the ssid name for each of the network.
 * [{"ssid"::"string value"},{"ssid":"string value"}]
 *
 * @return Pointer to the json list.
 */
char *get_available_networks_list();

/**
 * @brief is_wifi_connected
 * Check either the device is connected to a router or not.
 *
 * @return true: If thge device is connected.
 *         false: If the device is disconnected.
 */
bool is_wifi_connected();

/**
 * @brief update_sta_config
 * Update credentials for sta mopde of device to connect it to a router.
 *
 * @param config: wifi_config_t type
 */
void update_sta_config(wifi_config_t config);


#endif /* WIFI_MANAGER_H*/
