#pragma once

#define DEFAULT_AP_IP "192.168.4.1"
/**
 * @brief It starts freeRTOS task for DNS server.
 */
void dns_start();

/**
 * @brief Deletes freeRTOS task of DNS server.
 */
void dns_stop();

