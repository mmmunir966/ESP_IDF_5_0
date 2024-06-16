#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_netif.h"
#include <esp_http_server.h>
#include "wifi_manager.h"
#include "dns_server.h"


/** 
 * @brief start the web server 
 */
void start_web_server();

/**
 * @brief stops the web server 
 */
void stop_web_server();

#endif
