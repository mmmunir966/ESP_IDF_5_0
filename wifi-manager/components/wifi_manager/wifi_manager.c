#include "wifi_manager.h"

static const int WIFI_CONNECTED_BIT = BIT0; // ok
static const int WIFI_CONNECTING_BIT = BIT1;
static const int AP_STARTED_BIT = BIT2;
static const int AP_START_REQ_BIT = BIT3;
static const int STA_CONFIG_REQ_BIT = BIT4;
static const int WIFI_SCANNING_BIT = BIT5;
static const int CLIENT_CONNECTED_BIT = BIT6;

#define IS_WIFI_CONNECTED_BIT(x) (x & WIFI_CONNECTED_BIT)
#define IS_AP_STARTED_BIT(x) (x & AP_STARTED_BIT)
#define IS_STA_CONFIG_REQ_BIT(x) (x & STA_CONFIG_REQ_BIT)
#define IS_WIFI_CONNECTING_BIT(x) (x & WIFI_CONNECTING_BIT)
#define IS_WIFI_SCANNING_BIT(x) (x & WIFI_SCANNING_BIT)
#define IS_AP_START_REQ_BIT(x) (x & AP_START_REQ_BIT)
#define IS_CLIENT_CONNECTED_BIT(x) (x & CLIENT_CONNECTED_BIT)

static const char TAG[] = "wifi_manager";
static const char wifi_nvs_namespace[] = "espwifimgr";
static const char *WIFI_SSID_KEY = "ssid";
static const char *WIFI_PASSWORD_KEY = "password";
static char *ap_json_list = NULL;

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;
static EventGroupHandle_t wifi_status_events_group = NULL;

static uint16_t scanned_aps_count = 0;
static wifi_ap_record_t *scanned_aps_list = NULL;
static wifi_config_t *wifi_sta_config = NULL;

static SemaphoreHandle_t wifi_connectivity_mutex = NULL;
static SemaphoreHandle_t wifi_list_mutex = NULL;
static SemaphoreHandle_t nvs_mutex = NULL;

static void wifi_event_handler(void *args, esp_event_base_t base_event, int32_t event_id, void *event_data);
static esp_err_t save_sta_config(wifi_config_t config);
static void update_networks_list();
static bool load_wifi_sta_config();
static void enable_ap_sta_modes();
static void stop_esp_wifi();

void start_wifi_manager()
{
	wifi_status_events_group = xEventGroupCreate();
	wifi_connectivity_mutex = xSemaphoreCreateMutex();
	wifi_list_mutex = xSemaphoreCreateMutex();
	nvs_mutex = xSemaphoreCreateMutex();

	/* initialize the tcp stack */
	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	sta_netif = esp_netif_create_default_wifi_sta();
	ap_netif = esp_netif_create_default_wifi_ap();

	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

	esp_event_handler_instance_t wifi_event_handle;
	esp_event_handler_instance_t ip_event_handle;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_event_handle));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &ip_event_handle));

	xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTED_BIT);
	xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTING_BIT);
	xEventGroupClearBits(wifi_status_events_group, AP_STARTED_BIT);
	xEventGroupClearBits(wifi_status_events_group, AP_START_REQ_BIT);
	xEventGroupClearBits(wifi_status_events_group, STA_CONFIG_REQ_BIT);
	xEventGroupClearBits(wifi_status_events_group, CLIENT_CONNECTED_BIT);

	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_start();
}

void stop_wifi_manager()
{
	stop_esp_wifi();
	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
	esp_netif_destroy_default_wifi(sta_netif);
	esp_netif_destroy_default_wifi(ap_netif);
}

static void perform_network_scanning()
{
	vTaskDelay(pdMS_TO_TICKS(1000));
	EventBits_t evtGrpBits = xEventGroupGetBits(wifi_status_events_group);
	if (IS_CLIENT_CONNECTED_BIT(evtGrpBits) && !IS_WIFI_SCANNING_BIT(evtGrpBits) && !IS_WIFI_SCANNING_BIT(evtGrpBits))
	{
		ESP_LOGI(TAG, "Performing scan.");
		xEventGroupSetBits(wifi_status_events_group, WIFI_SCANNING_BIT);
		wifi_scan_config_t config_scan = {.ssid = 0, .bssid = 0, .channel = 0, .show_hidden = true};
		esp_wifi_scan_start(&config_scan, false);
	}
}

static wifi_config_t *get_wifi_sta_config()
{
	return wifi_sta_config;
}

static void connect_wifi()
{
	EventBits_t evtGrpBits = xEventGroupGetBits(wifi_status_events_group);
	if (IS_WIFI_SCANNING_BIT(evtGrpBits) || IS_WIFI_CONNECTING_BIT(evtGrpBits))
		return;

	vTaskDelay(pdMS_TO_TICKS(2000));
	if (xSemaphoreTake(wifi_connectivity_mutex, portMAX_DELAY) != pdTRUE)
	{
		ESP_LOGE(TAG, "Error getting wifi_connectivity_mutex.");
		return;
	}

	if (!IS_WIFI_CONNECTED_BIT(evtGrpBits))
	{
		xEventGroupClearBits(wifi_status_events_group, STA_CONFIG_REQ_BIT);

		if (load_wifi_sta_config())
		{
			xEventGroupSetBits(wifi_status_events_group, WIFI_CONNECTING_BIT);
			ESP_LOGI(TAG, "Saved wifi credentials found. Attempting to connect.");
			ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, get_wifi_sta_config()));
			esp_wifi_connect();
		}
		else
		{
			ESP_LOGI(TAG, "No credentials found for wifi sta mode.");
			enable_ap_sta_modes();
			perform_network_scanning();
		}
	}
	else if (IS_STA_CONFIG_REQ_BIT(evtGrpBits)) // This will happen when credentials are updated.
	{
		ESP_LOGI(TAG, "Performing disconnect due to STA_CONFIG_REQ");
		xEventGroupClearBits(wifi_status_events_group, STA_CONFIG_REQ_BIT);
		esp_wifi_disconnect();
	}

	xSemaphoreGive(wifi_connectivity_mutex);
}

void wifi_event_handler(void *args, esp_event_base_t base_event, int32_t event_id, void *event_data)
{
	if (base_event == WIFI_EVENT)
	{
		switch (event_id)
		{
		case WIFI_EVENT_SCAN_DONE:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
			xEventGroupClearBits(wifi_status_events_group, WIFI_SCANNING_BIT);
			update_networks_list();
			connect_wifi();
			break;
		}

		case WIFI_EVENT_STA_START:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
			connect_wifi();
			break;
		}

		case WIFI_EVENT_STA_DISCONNECTED:
		{
			ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED");
			xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTED_BIT);
			xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTING_BIT);

			EventBits_t evtGrpBits = xEventGroupGetBits(wifi_status_events_group);
			if (!IS_AP_STARTED_BIT(evtGrpBits))
				enable_ap_sta_modes();

			if (IS_CLIENT_CONNECTED_BIT(evtGrpBits))
				perform_network_scanning();
			else
				connect_wifi();

			break;
		}

		case WIFI_EVENT_AP_START:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
			xEventGroupSetBits(wifi_status_events_group, AP_STARTED_BIT);
			xEventGroupClearBits(wifi_status_events_group, AP_START_REQ_BIT);

			EventBits_t evtGrpBits = xEventGroupGetBits(wifi_status_events_group);
			if (IS_CLIENT_CONNECTED_BIT(evtGrpBits))
				perform_network_scanning();
			else
				connect_wifi();

			break;
		}

		case WIFI_EVENT_AP_STOP:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
			xEventGroupClearBits(wifi_status_events_group, AP_STARTED_BIT);
			break;
		}
		case WIFI_EVENT_AP_STACONNECTED:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
			xEventGroupSetBits(wifi_status_events_group, CLIENT_CONNECTED_BIT);
			connect_wifi();
			break;
		}
		case WIFI_EVENT_AP_STADISCONNECTED:
		{
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
			xEventGroupClearBits(wifi_status_events_group, CLIENT_CONNECTED_BIT);
			break;
		}
		default:
			break;
		}
	}
	else if (base_event == IP_EVENT)
	{
		switch (event_id)
		{
		case IP_EVENT_STA_GOT_IP:
		{
			ESP_LOGD(TAG, "IP_EVENT_STA_GOT_IP");
			xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTING_BIT);
			xEventGroupSetBits(wifi_status_events_group, WIFI_CONNECTED_BIT);
			esp_wifi_set_mode(WIFI_MODE_STA);

			break;
		}

		default:
			break;
		}
	}
}

void enable_ap_sta_modes()
{
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);

	if (mode == WIFI_MODE_APSTA)
	{
		ESP_LOGW(TAG, "AP mode is already open, please connect to it to update wifi credentials.");
		return;
	}

	wifi_config_t config_ap = {
		.ap = {
			.ssid = DEFAULT_AP_SSID,
			.channel = DEFAULT_AP_CHANNEL,
			.ssid_hidden = DEFAULT_AP_SSID_HIDDEN, // visibility of the access point. 0: visible AP. 1: hidden
			.max_connection = DEFAULT_AP_MAX_CONNECTIONS,
			.beacon_interval = DEFAULT_AP_BEACON_INTERVAL,
		},
	};

	if (strlen((DEFAULT_AP_PASSWORD)) > 0)
	{
		config_ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
		memcpy(config_ap.ap.password, DEFAULT_AP_PASSWORD, strlen(DEFAULT_AP_PASSWORD));
	}
	else
	{
		config_ap.ap.authmode = WIFI_AUTH_OPEN;
		memset(config_ap.ap.password, 0x00, MAX_PASSPHRASE_LEN);
	}

	stop_esp_wifi();

	xEventGroupClearBits(wifi_status_events_group, WIFI_SCANNING_BIT);
	xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTED_BIT);

	ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
	esp_wifi_stop();
	esp_netif_ip_info_t netif_ip_info;
	memset(&netif_ip_info, 0x00, sizeof(netif_ip_info));
	inet_pton(AF_INET, DEFAULT_AP_IP, &netif_ip_info.ip);
	inet_pton(AF_INET, DEFAULT_AP_GATEWAY, &netif_ip_info.gw);
	inet_pton(AF_INET, DEFAULT_AP_NETMASK, &netif_ip_info.netmask);
	ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &netif_ip_info));
	ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

	esp_wifi_set_mode(WIFI_MODE_APSTA);
	esp_wifi_set_config(WIFI_IF_AP, &config_ap);
	esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	esp_wifi_start();
}

void update_sta_config(wifi_config_t config)
{
	save_sta_config(config);
	xEventGroupSetBits(wifi_status_events_group, STA_CONFIG_REQ_BIT);
}

void stop_esp_wifi()
{
	esp_wifi_stop();

	xEventGroupClearBits(wifi_status_events_group, WIFI_SCANNING_BIT);
	xEventGroupClearBits(wifi_status_events_group, WIFI_CONNECTED_BIT);
}

static void make_aps_list_unique()
{
	ESP_LOGI(TAG, "Removing duplicate Aps.");
	for (int i = 0; i < scanned_aps_count; i++)
	{
		if (scanned_aps_list[i].ssid == NULL)
		{
			continue;
		}

		for (int j = i + 1; j < scanned_aps_count; j++)
		{
			if (scanned_aps_list[j].ssid != NULL && strcmp((char *)scanned_aps_list[i].ssid, (char *)scanned_aps_list[j].ssid) == 0)
			{
				// Duplicate record found, remove it by shifting the remaining records
				for (int d = j; d < scanned_aps_count - 1; d++)
				{
					strcpy((char *)&scanned_aps_list[d], (char *)&scanned_aps_list[d + 1]);
				}
				scanned_aps_count--;
				j--;
			}
		}
	}
}

void update_networks_list()
{
	ESP_LOGI(TAG, "Updating networks list.");
	if (xSemaphoreTake(wifi_list_mutex, portMAX_DELAY) != pdTRUE)
	{
		ESP_LOGE(TAG, "Error getting wifi_list_mutex.");
		return;
	}

	esp_wifi_scan_get_ap_num(&scanned_aps_count);
	if (scanned_aps_count <= 0)
	{
		ESP_LOGW(TAG, "No AP is detected.");
		xSemaphoreGive(wifi_list_mutex);
		return;
	}

	if (scanned_aps_list)
		free(scanned_aps_list), scanned_aps_list = NULL;

	scanned_aps_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * scanned_aps_count);
	esp_wifi_scan_get_ap_records(&scanned_aps_count, scanned_aps_list);
	make_aps_list_unique();

	ESP_LOGI(TAG, "cJSON_CreateArray.");
	cJSON *json_array = cJSON_CreateArray();
	if (json_array != NULL)
	{
		cJSON *json_obj = NULL;

		for (int i = 0; i < scanned_aps_count; i++)
		{
			ESP_LOGI(TAG, "cJSON_CreateObjects.");
			ESP_LOGI(TAG, "ssid %d: %s", i, scanned_aps_list[i].ssid);
			json_obj = cJSON_CreateObject();
			if (json_obj)
			{
				cJSON_AddStringToObject(json_obj, "ssid", (char *)scanned_aps_list[i].ssid);
				cJSON_AddItemToArray(json_array, json_obj);
				// cJSON_Delete(json_obj), json_obj = NULL;
			}
		}
		ap_json_list = cJSON_Print(json_array);
		ESP_LOGI(TAG, "Updated List: %s", ap_json_list);
		xSemaphoreGive(wifi_list_mutex);
	}
}

char *get_available_networks_list()
{
	ESP_LOGI(TAG, "Getting network's list");
	if (scanned_aps_count <= 0)
	{
		ESP_LOGE(TAG, "Network list is not available.");
		return 0;
	}

	if (xSemaphoreTake(wifi_list_mutex, portMAX_DELAY) != pdTRUE)
	{
		ESP_LOGE(TAG, "Error getting wifi_list_mutex.");
		return 0;
	}

	size_t list_len = strlen(ap_json_list) * sizeof(char);
	char *scanned_list = (char *)malloc(list_len);
	memset(scanned_list, 0x00, list_len);
	ESP_LOGI(TAG, "Copy list.");
	strcpy(scanned_list, ap_json_list);
	ESP_LOGI(TAG, "Network's list is %s", scanned_list);
	xSemaphoreGive(wifi_list_mutex);

	return scanned_list;
}

esp_err_t save_sta_config(wifi_config_t config)
{
	nvs_handle handle;
	esp_err_t esp_err;
	size_t sz;

	ESP_LOGI(TAG, "Saving config to flash.");

	if (nvs_mutex && xSemaphoreTake(nvs_mutex, portMAX_DELAY))
	{
		esp_err = nvs_open(wifi_nvs_namespace, NVS_READWRITE, &handle);
		if (esp_err != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return esp_err;
		}

		esp_err = nvs_set_blob(handle, WIFI_SSID_KEY, config.sta.ssid, MAX_SSID_LEN);
		if (esp_err != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return esp_err;
		}

		esp_err = nvs_set_blob(handle, WIFI_PASSWORD_KEY, config.sta.password, MAX_PASSPHRASE_LEN);
		if (esp_err != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return esp_err;
		}

		ESP_LOGI(TAG, "Config saved to flash.");
		xSemaphoreGive(nvs_mutex);
	}
	else
	{
		ESP_LOGE(TAG, "save_sta_config failed to acquire nvs_mutex");
	}

	return ESP_OK;
}

bool load_wifi_sta_config()
{
	nvs_handle handle;
	esp_err_t esp_err;
	if (nvs_mutex && xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE)
	{
		if (nvs_open(wifi_nvs_namespace, NVS_READONLY, &handle) != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return false;
		}

		if (wifi_sta_config == NULL)
		{
			wifi_sta_config = (wifi_config_t *)malloc(sizeof(wifi_config_t));
		}
		memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));

		char ssid_value[MAX_SSID_LEN] = {0};
		size_t size = MAX_SSID_LEN;
		esp_err = nvs_get_blob(handle, WIFI_SSID_KEY, ssid_value, &size);
		if (esp_err != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return false;
		}
		ESP_LOGD(TAG, "Loaded ssid: %s", ssid_value);
		memcpy(wifi_sta_config->sta.ssid, ssid_value, size);

		char password_value[MAX_PASSPHRASE_LEN] = {0};
		size = MAX_PASSPHRASE_LEN;
		esp_err = nvs_get_blob(handle, WIFI_PASSWORD_KEY, password_value, &size);
		if (esp_err != ESP_OK)
		{
			xSemaphoreGive(nvs_mutex);
			return false;
		}
		ESP_LOGD(TAG, "Loaded pwd: %s", password_value);
		memcpy(wifi_sta_config->sta.password, password_value, size);
		ESP_LOGI(TAG, "load_wifi_sta_config: ssid:%s password:%s", wifi_sta_config->sta.ssid, wifi_sta_config->sta.password);
		xSemaphoreGive(nvs_mutex);
		return true;
	}
	else
	{
		ESP_LOGE(TAG, "load_wifi_sta_config failed to acquire nvs_mutex");
	}
	return false;
}
