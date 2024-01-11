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

#include "esp_blufi.h"
///////////////////
#include <stdint.h>
#include <stddef.h>
#include "esp_netif.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "driver/uart.h"
#include "driver/gpio.h"

///////////////////////////////////////////////////
#include "web_server.h"

/////////////////////////////////////////////

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
		const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, pdMS_TO_TICKS(1000));
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
			vTaskDelay(20 / portTICK_PERIOD_MS);
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
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
	mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{

	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = CONFIG_BROKER_URI,
		.cert_pem = (const char *)mqtt_eclipseprojects_io_pem_start,
	};

	ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
								   client);
	esp_mqtt_client_start(client);
}

void app_main(void)
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
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
	start_wifi_manager();
	start_web_server();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	uart_init();
	xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES,
				NULL);

	while (is_wifi_connected() == false)
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
