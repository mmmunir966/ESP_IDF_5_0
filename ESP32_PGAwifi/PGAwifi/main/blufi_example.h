#pragma once
#include "esp_blufi_api.h"

#define BLUFI_EXAMPLE_TAG "BLUFI_PGA"
//#define PGA_BIN_NUMBER "/PGA_BIN/CT1005"
#define BLUFI_DEVICE_PGA    "BIN_CT"

#define BLUFI_INFO(fmt, ...)   ESP_LOGI(BLUFI_EXAMPLE_TAG, fmt, ##__VA_ARGS__)
#define BLUFI_ERROR(fmt, ...)  ESP_LOGE(BLUFI_EXAMPLE_TAG, fmt, ##__VA_ARGS__)

void blufi_dh_negotiate_data_handler(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);
int blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
int blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len);

int blufi_security_init(void);
void blufi_security_deinit(void);
int esp_blufi_gap_register_callback(void);
esp_err_t esp_blufi_host_init(void);
static void initialise_wifi(void);
static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);
