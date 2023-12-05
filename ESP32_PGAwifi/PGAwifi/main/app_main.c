/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
 * This is a demo for bluetooth config wifi connection to ap. You can config ESP32 to connect a softap
 * or config ESP32 as a softap to be connected by other device. APP can be downloaded from github
 * android source code: https://github.com/EspressifApp/EspBlufi
 * iOS source code: https://github.com/EspressifApp/EspBlufiForiOS
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_blufi_api.h"
#include "blufi_example.h"

#include "esp_blufi.h"
///////////////////
#include <stdint.h>
#include <stddef.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "driver/uart.h"
#include "driver/gpio.h"
/////////////
#include "blufi_example.h"
#include <stdlib.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_bt.h"

#include "esp_blufi_api.h"
#include "esp_blufi.h"
/////////////////////////////
static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

#define WIFI_LIST_NUM 10

static wifi_config_t sta_config;
static wifi_config_t ap_config;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* store the station info for send back to phone */
static bool gl_sta_connected = false;
static bool ble_is_connected = false;
static uint8_t gl_sta_bssid[6];
static uint8_t gl_sta_ssid[32];
static int gl_sta_ssid_len;
///////////////////////////////////////////////////

static const char *TAG = "MQTTS_PGA";
static char PGA_BIN_NUMBER[27];

static char name_ID[16];
static bool name_ID_flag = false;
static const int RX_BUF_SIZE = 3000;
int rxflag = 0;
esp_mqtt_client_handle_t client;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define Tx_length 12

#define GPIO_OUTPUT_IO_0 32
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT_IO_0)

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[] = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[] asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
// extern const uint8_t mqtt_eclipseprojects_io_pem_end[] asm("_binary_mqtt_eclipseprojects_io_pem_end");
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void ip_event_handler(void *arg, esp_event_base_t event_base,
							 int32_t event_id, void *event_data)
{
	wifi_mode_t mode;

	switch (event_id)
	{
	case IP_EVENT_STA_GOT_IP:
	{
		esp_blufi_extra_info_t info;

		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		esp_wifi_get_mode(&mode);

		memset(&info, 0, sizeof(esp_blufi_extra_info_t));
		memcpy(info.sta_bssid, gl_sta_bssid, 6);
		info.sta_bssid_set = true;
		info.sta_ssid = gl_sta_ssid;
		info.sta_ssid_len = gl_sta_ssid_len;
		if (ble_is_connected == true)
		{
			esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
		}
		else
		{
			BLUFI_INFO("BLUFI BLE is not connected1 yet\n");
		}
		break;
	}
	default:
		break;
	}
	return;
}


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
							   int32_t event_id, void *event_data)
{
	wifi_event_sta_connected_t *event;
	wifi_mode_t mode;

	switch (event_id)
	{
	case WIFI_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_CONNECTED:
		gl_sta_connected = true;
		BLUFI_INFO("WIFI connected\n");
		event = (wifi_event_sta_connected_t *)event_data;
		memcpy(gl_sta_bssid, event->bssid, 6);
		memcpy(gl_sta_ssid, event->ssid, event->ssid_len);
		gl_sta_ssid_len = event->ssid_len;
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		   auto-reassociate. */
		gl_sta_connected = false;
		BLUFI_INFO("WIFI disconnected\n");
		memset(gl_sta_ssid, 0, 32);
		memset(gl_sta_bssid, 0, 6);
		gl_sta_ssid_len = 0;
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	case WIFI_EVENT_AP_START:
		esp_wifi_get_mode(&mode);

		/* TODO: get config or information of softap, then set to report extra_info */
		if (ble_is_connected == true)
		{
			if (gl_sta_connected)
			{
				esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL);
			}
			else
			{
				esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
			}
		}
		else
		{
			BLUFI_INFO("BLUFI BLE is not connected2 yet\n");
		}
		break;
	case WIFI_EVENT_SCAN_DONE:
	{
		uint16_t apCount = 0;
		esp_wifi_scan_get_ap_num(&apCount);
		if (apCount == 0)
		{
			BLUFI_INFO("Nothing AP found");
			break;
		}
		wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
		if (!ap_list)
		{
			BLUFI_ERROR("malloc error, ap_list is NULL");
			break;
		}
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
		esp_blufi_ap_record_t *blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
		if (!blufi_ap_list)
		{
			if (ap_list)
			{
				free(ap_list);
			}
			BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
			break;
		}
		for (int i = 0; i < apCount; ++i)
		{
			blufi_ap_list[i].rssi = ap_list[i].rssi;
			memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
		}

		if (ble_is_connected == true)
		{
			esp_blufi_send_wifi_list(apCount, blufi_ap_list);
		}
		else
		{
			BLUFI_INFO("BLUFI BLE is not connected3 yet\n");
		}

		esp_wifi_scan_stop();
		free(ap_list);
		free(blufi_ap_list);
		break;
	}
	default:
		break;
	}
	return;
}

static void initialise_wifi(void)
{
	ESP_ERROR_CHECK(esp_netif_init());
	wifi_event_group = xEventGroupCreate();
	//   ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_blufi_callbacks_t example_callbacks = {
	.event_cb = example_event_callback,
	.negotiate_data_handler = blufi_dh_negotiate_data_handler,
	.encrypt_func = blufi_aes_encrypt,
	.decrypt_func = blufi_aes_decrypt,
	.checksum_func = blufi_crc_checksum,
};

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
	/* actually, should post to blufi_task handle the procedure,
	 * now, as a example, we do it more simply */
	switch (event)
	{
	case ESP_BLUFI_EVENT_INIT_FINISH:
		BLUFI_INFO("BLUFI init finish\n");

		//       esp_ble_gap_set_device_name(BLUFI_DEVICE_PGA);
		esp_blufi_adv_start();
		break;
	case ESP_BLUFI_EVENT_DEINIT_FINISH:
		BLUFI_INFO("BLUFI deinit finish\n");
		break;
	case ESP_BLUFI_EVENT_BLE_CONNECT:
		BLUFI_INFO("BLUFI ble connect\n");
		ble_is_connected = true;
		esp_blufi_adv_stop();
		blufi_security_init();
		break;
	case ESP_BLUFI_EVENT_BLE_DISCONNECT:
		BLUFI_INFO("BLUFI ble disconnect\n");
		ble_is_connected = false;
		blufi_security_deinit();
		esp_blufi_adv_start();
		break;
	case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
		BLUFI_INFO("BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
		ESP_ERROR_CHECK(esp_wifi_set_mode(param->wifi_mode.op_mode));
		break;
	case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
		BLUFI_INFO("BLUFI requset wifi connect to AP\n");
		/* there is no wifi callback when the device has already connected to this wifi
		so disconnect wifi before connection.
		*/
		esp_wifi_disconnect();
		esp_wifi_connect();
		break;
	case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
		BLUFI_INFO("BLUFI requset wifi disconnect from AP\n");
		esp_wifi_disconnect();
		break;
	case ESP_BLUFI_EVENT_REPORT_ERROR:
		BLUFI_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
		esp_blufi_send_error_info(param->report_error.state);
		break;
	case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
	{
		wifi_mode_t mode;
		esp_blufi_extra_info_t info;

		esp_wifi_get_mode(&mode);

		if (gl_sta_connected)
		{
			memset(&info, 0, sizeof(esp_blufi_extra_info_t));
			memcpy(info.sta_bssid, gl_sta_bssid, 6);
			info.sta_bssid_set = true;
			info.sta_ssid = gl_sta_ssid;
			info.sta_ssid_len = gl_sta_ssid_len;
			esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
		}
		else
		{
			esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
		}
		BLUFI_INFO("BLUFI get wifi status from AP\n");

		break;
	}
	case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
		BLUFI_INFO("blufi close a gatt connection");
		esp_blufi_disconnect();
		break;
	case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
		/* TODO */
		break;
	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
		memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
		sta_config.sta.bssid_set = 1;
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		BLUFI_INFO("Recv STA BSSID %s\n", sta_config.sta.ssid);
		break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
		strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
		sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		BLUFI_INFO("Recv STA SSID %s\n", sta_config.sta.ssid);
		break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
		strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
		sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		BLUFI_INFO("Recv STA PASSWORD %s\n", sta_config.sta.password);
		break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
		strncpy((char *)ap_config.ap.ssid, (char *)param->softap_ssid.ssid, param->softap_ssid.ssid_len);
		ap_config.ap.ssid[param->softap_ssid.ssid_len] = '\0';
		ap_config.ap.ssid_len = param->softap_ssid.ssid_len;
		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		BLUFI_INFO("Recv SOFTAP SSID %s, ssid len %d\n", ap_config.ap.ssid, ap_config.ap.ssid_len);
		break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
		strncpy((char *)ap_config.ap.password, (char *)param->softap_passwd.passwd, param->softap_passwd.passwd_len);
		ap_config.ap.password[param->softap_passwd.passwd_len] = '\0';
		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		BLUFI_INFO("Recv SOFTAP PASSWORD %s len = %d\n", ap_config.ap.password, param->softap_passwd.passwd_len);
		break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
		if (param->softap_max_conn_num.max_conn_num > 4)
		{
			return;
		}
		ap_config.ap.max_connection = param->softap_max_conn_num.max_conn_num;
		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		BLUFI_INFO("Recv SOFTAP MAX CONN NUM %d\n", ap_config.ap.max_connection);
		break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
		if (param->softap_auth_mode.auth_mode >= WIFI_AUTH_MAX)
		{
			return;
		}
		ap_config.ap.authmode = param->softap_auth_mode.auth_mode;
		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		BLUFI_INFO("Recv SOFTAP AUTH MODE %d\n", ap_config.ap.authmode);
		break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
		if (param->softap_channel.channel > 13)
		{
			return;
		}
		ap_config.ap.channel = param->softap_channel.channel;
		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		BLUFI_INFO("Recv SOFTAP CHANNEL %d\n", ap_config.ap.channel);
		break;
	case ESP_BLUFI_EVENT_GET_WIFI_LIST:
	{
		wifi_scan_config_t scanConf = {
			.ssid = NULL,
			.bssid = NULL,
			.channel = 0,
			.show_hidden = false};
		esp_wifi_scan_start(&scanConf, true);
		break;
	}
	case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
		BLUFI_INFO("Recv Custom Data %\n", param->custom_data.data_len);
		esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);
		break;
	case ESP_BLUFI_EVENT_RECV_USERNAME:
		/* Not handle currently */
		break;
	case ESP_BLUFI_EVENT_RECV_CA_CERT:
		/* Not handle currently */
		break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
		/* Not handle currently */
		break;
	case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
		/* Not handle currently */
		break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
		/* Not handle currently */
		break;
		;
	case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
		/* Not handle currently */
		break;
	default:
		break;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/* MQTT over SSL Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////end ble//////////////////////////////////////////////////////////////////////////////

//
// Note: this function is for testing purposes only publishing part of the active partition
//       (to be checked against the original binary)
//

/////////////////////////////////////Uart//////////////////////
void uart_init(void)
{
	const uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits =
			UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits =
			UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};
	// We won't use a buffer for sending data.
	uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_2, &uart_config);
	uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
				 UART_PIN_NO_CHANGE);
}

int sendData(const char *logName, char *data)
{
	int len = strlen(data);
	if (len >= Tx_length)
		len = Tx_length;
	else
	{
		while (len == Tx_length)
		{
			data[len] = 0;
			len++;
		}
	}
	const int txBytes = uart_write_bytes(UART_NUM_2, data, len);
	ESP_LOGI(logName, "Wrote %d bytes", txBytes);
	return txBytes;
}
void tx_task_MCU(char *data)
{
	static const char *TX_TASK_TAG = "TX_TASK";
	char *dataT = (char *)data;
	esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
	sendData(TX_TASK_TAG, dataT);
	vTaskDelay(2000 / portTICK_PERIOD_MS);
}

char Hex2Str(uint8_t dat, int idx)
{
	uint8_t temp = 0;
	if (idx)
		temp = dat >> 4;
	else
		temp = dat & 0xf;
	if (temp <= 9)
		temp += '0';
	else
		temp = temp - 10 + 'A';
	return (char)temp;
}
void AllHex2Str(char *dest, uint8_t *src, int rxBytes)
{
	int i = 0;

	for (i = 0; i < rxBytes; i++)
	{
		if (i % 2)
			dest[i] = Hex2Str(src[i / 2], 0);
		else
			dest[i] = Hex2Str(src[i / 2], 1);
	}
}

static void rx_task(void *arg)
{
	static const char *RX_TASK_TAG = "RX_TASK";
	esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
	uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
	char *dataToserver = (char *)malloc(RX_BUF_SIZE * 2 + 1);
	char *dataToMcu = "transferredd";
	char *PGA_BIN_temp = "/PGA_BIN/CT";
	int i;
	char *PGA_BIN_NUMBER1 = (char *)malloc(27);

	int length;
	while (1)
	{
		const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE,
											1000 / portTICK_RATE_MS);
		length = rxBytes * 2;
		AllHex2Str(dataToserver, data, length);
		if (rxBytes > 0)
		{
			if ((data[0] == 0x7E) && (length = 16))
			{
				tx_task_MCU(dataToMcu);
				for (i = 0; i < 11; i++)
					PGA_BIN_NUMBER1[i] = PGA_BIN_temp[i];
				for (i = 0; i < 16; i++)
				{
					name_ID[i] = dataToserver[i + 14];
					PGA_BIN_NUMBER1[i + 11] = name_ID[i];
				}
				for (i = 0; i < 27; i++)
					PGA_BIN_NUMBER[i] = PGA_BIN_NUMBER1[i];
				name_ID_flag = true;
				ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, name_ID, 16,
									   ESP_LOG_INFO);
				ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, PGA_BIN_NUMBER, 27,
									   ESP_LOG_INFO);
			}
			else
			{
				data[rxBytes] = 0;
				ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
				ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes,
									   ESP_LOG_INFO);
				esp_mqtt_client_publish(client, PGA_BIN_NUMBER, dataToserver,
										length, 0, 0);
				tx_task_MCU(dataToMcu);
			}
		}
	}
	free(data);
	free(dataToserver);
}
////////////////////////////////////////////end uart///////////////////////////////////////////////////
//////////////////////////////
void GPIO_init(void)
{
	gpio_config_t io_conf;
	// disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	// bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	// disable pull-down mode
	io_conf.pull_down_en = 0;
	// disable pull-up mode
	io_conf.pull_up_en = 0;
	// configure GPIO with the given settings
	gpio_config(&io_conf);
}
///////////////////////////////
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	char *dataToMcu;
	// your_context_t *context = event->context;
	switch (event->event_id)
	{
	case MQTT_EVENT_CONNECTED:
		dataToMcu = "connectedddd";
		tx_task_MCU(dataToMcu);
		gpio_set_level(GPIO_OUTPUT_IO_0, 0);

		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		msg_id = esp_mqtt_client_subscribe(client, PGA_BIN_NUMBER, 0);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
		/*
		 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		 ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		 msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		 ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		 */
		//           xTaskCreate(GPIO_LED_Task, "GPIO_LED_Task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		dataToMcu = "disconnected";
		tx_task_MCU(dataToMcu);
		gpio_set_level(GPIO_OUTPUT_IO_0, 1);
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		//            msg_id = esp_mqtt_client_publish(client, "/Home", "data: hello Hamza", 0, 0, 0);
		//            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		dataToMcu = "subscribeddd";
		tx_task_MCU(dataToMcu);
		gpio_set_level(GPIO_OUTPUT_IO_0, 0);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		dataToMcu = "unsubscribed";
		tx_task_MCU(dataToMcu);
		//		gpio_set_level(GPIO_OUTPUT_IO_0, 0);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		dataToMcu = "publishedddd";
		tx_task_MCU(dataToMcu);
		gpio_set_level(GPIO_OUTPUT_IO_0, 0);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		int cnt, cnt0;
		cnt = 0;
		cnt0 = event->data_len;
		if (cnt0 > 100)
			cnt0 = cnt0 / 20;

		while (cnt0)
		{
			printf("cnt: %d\n", cnt++);
			vTaskDelay(20 / portTICK_RATE_MS);
			gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
			cnt0--;
		}
		gpio_set_level(GPIO_OUTPUT_IO_0, 0);

		if (strncmp(event->data, "request data", event->data_len - 4) == 0)
		{
			ESP_LOGI(TAG, "Sending the binary");
			dataToMcu = event->data;
			tx_task_MCU(dataToMcu);
			//                send_binary(client);
		}
		if (strncmp(event->data, "setFANonBINX", event->data_len - 8) == 0)
		{
			ESP_LOGI(TAG, "Sending the binary");
			dataToMcu = event->data;
			tx_task_MCU(dataToMcu);
		}
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
		{
			ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x",
					 event->error_handle->esp_tls_last_esp_err);
			ESP_LOGI(TAG, "Last tls stack error number: 0x%x",
					 event->error_handle->esp_tls_stack_err);
			ESP_LOGI(TAG, "Last captured errno : %d (%s)",
					 event->error_handle->esp_transport_sock_errno,
					 strerror(event->error_handle->esp_transport_sock_errno));
		}
		else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
		{
			ESP_LOGI(TAG, "Connection refused error: 0x%x",
					 event->error_handle->connect_return_code);
		}
		else
		{
			ESP_LOGW(TAG, "Unknown error type: 0x%x",
					 event->error_handle->error_type);
		}

		dataToMcu = "event_error ";
		tx_task_MCU(dataToMcu);
		//		gpio_set_level(GPIO_OUTPUT_IO_0, 1);
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
	return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
							   int32_t event_id, void *event_data)
{
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
			 event_id);
	mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = CONFIG_BROKER_URI,
		.cert_pem = (const char *)mqtt_eclipseprojects_io_pem_start,
	};

	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
								   client);
	esp_mqtt_client_start(client);
}

void app_main(void)
{
	esp_err_t ret;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
	esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
	esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	initialise_wifi();

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret)
	{
		BLUFI_ERROR("%s initialize bt controller failed: %s\n", __func__, esp_err_to_name(ret));
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret)
	{
		BLUFI_ERROR("%s enable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_blufi_host_init();
	if (ret)
	{
		BLUFI_ERROR("%s initialise host failed: %s\n", __func__, esp_err_to_name(ret));
		return;
	}

	BLUFI_INFO("BLUFI VERSION %04x\n", esp_blufi_get_version());

	ret = esp_blufi_register_callbacks(&example_callbacks);
	if (ret)
	{
		BLUFI_ERROR("%s blufi register failed, error code = %x\n", __func__, ret);
		return;
	}

	ret = esp_blufi_gap_register_callback();
	if (ret)
	{
		BLUFI_ERROR("%s gap register failed, error code = %x\n", __func__, ret);
		return;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	uart_init();
	xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES,
				NULL);

	while (gl_sta_connected == false)
	{
		vTaskDelay(1);
	}
	while (name_ID_flag == false)
	{
		vTaskDelay(1);
	}
	GPIO_init();

	mqtt_app_start();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
