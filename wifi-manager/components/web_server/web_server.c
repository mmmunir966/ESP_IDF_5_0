#include "web_server.h"

static const char TAG[] = "web_server";
static httpd_handle_t httpd_handle = NULL;

esp_err_t (*custom_get_httpd_uri_handler)(httpd_req_t *r) = NULL;
esp_err_t (*custom_post_httpd_uri_handler)(httpd_req_t *r) = NULL;

/* strings holding the URLs of the wifi manager */
static char *http_index_html_url = "/";
static char *http_styles_css_url = "/styles.css";
static char *http_common_js_url = "/common.js";
static char *http_logo_url = "/logo.png";
static char *http_update_wifi_cr_url = "/updateWiFi";
static char *http_get_networks_url = "/getAvailableNetworks";

/* Embedd html file*/
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t styles_css_start[] asm("_binary_styles_css_start");
extern const uint8_t styles_css_end[] asm("_binary_styles_css_end");
extern const uint8_t common_js_start[] asm("_binary_common_js_start");
extern const uint8_t common_js_end[] asm("_binary_common_js_end");
extern const uint8_t logo_start[] asm("_binary_logo_png_start");
extern const uint8_t logo_end[] asm("_binary_logo_png_end");

/* const httpd related values stored in ROM */
static const char http_200_hdr[] = "200 OK";
static const char http_404_hdr[] = "404 Not Found";
static const char contentType_png[] = "image/png";
static const char http_cache_control_hdr[] = "Cache-Control";
static const char http_cache_control_no_cache[] = "no-store, no-cache, must-revalidate, max-age=0";

static const char *WIFI_SSID_KEY = "ssid";
static const char *WIFI_PASSWORD_KEY = "password";

static const char HTTPD_TYPE_JS[] = "text/javascript";
static const char HTTPD_TYPE_CSS[] = "text/css";

static esp_err_t http_get_request_handler(httpd_req_t *req);
static esp_err_t http_post_request_handler(httpd_req_t *req);
/* URI wild card for any GET request */
static const httpd_uri_t httpd_register_get_request = {
	.uri = "*",
	.method = HTTP_GET,
	.handler = http_get_request_handler};

static const httpd_uri_t httpd_register_post_request = {
	.uri = "*",
	.method = HTTP_POST,
	.handler = http_post_request_handler};

void start_web_server()
{
	if (httpd_handle == NULL) // Start server only if it is not running already.
	{
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();

		config.uri_match_fn = httpd_uri_match_wildcard;
		config.lru_purge_enable = true;

		if (httpd_start(&httpd_handle, &config) == ESP_OK)
		{
			ESP_LOGI(TAG, "Registering URI handlers");
			httpd_register_uri_handler(httpd_handle, &httpd_register_get_request);
			httpd_register_uri_handler(httpd_handle, &httpd_register_post_request);
		}
	}
}

void stop_web_server()
{
	if (httpd_handle != NULL)
	{
		httpd_stop(httpd_handle), httpd_handle = NULL;
	}
}

static esp_err_t http_post_request_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "POST %s", req->uri);

	if (strcmp(req->uri, http_update_wifi_cr_url) == 0)
	{
		char *ssid = NULL;
		char *password = NULL;
		size_t ssid_len = 0, password_len = 0;

		ssid_len = httpd_req_get_hdr_value_len(req, WIFI_SSID_KEY);
		password_len = httpd_req_get_hdr_value_len(req, WIFI_PASSWORD_KEY);

		ESP_LOGD(TAG, "Updating wifi credentials with ssid_len %d and password_len %d.", ssid_len, password_len);

		if (ssid_len && ssid_len <= MAX_SSID_LEN && password_len && password_len <= MAX_PASSPHRASE_LEN)
		{
			/* get the actual value of the headers */
			ssid = (char *)malloc(sizeof(char) * (ssid_len + 1));
			password = (char *)malloc(sizeof(char) * (password_len + 1));
			httpd_req_get_hdr_value_str(req, WIFI_SSID_KEY, ssid, ssid_len + 1);
			httpd_req_get_hdr_value_str(req, WIFI_PASSWORD_KEY, password, password_len + 1);

			wifi_config_t config;
			memset(&config, 0x00, sizeof(wifi_config_t));
			memcpy(config.sta.ssid, ssid, ssid_len);
			memcpy(config.sta.password, password, password_len);
			ESP_LOGI(TAG, "Received ssid: %s", ssid);
			update_sta_config(config);

			httpd_resp_set_status(req, http_200_hdr);
			httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
			httpd_resp_send(req, NULL, 0);

			free(ssid), ssid = NULL;
			free(password), password = NULL;
			return ESP_OK;
		}
	}

	httpd_resp_set_status(req, http_404_hdr);
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}

static esp_err_t http_get_request_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "GET %s", req->uri);

	if (strcmp(req->uri, http_index_html_url) == 0)
	{
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
		httpd_resp_send(req, (char *)index_html_start, index_html_end - index_html_start);
	}
	else if (strcmp(req->uri, http_styles_css_url) == 0)
	{
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_CSS);
		httpd_resp_send(req, (char *)styles_css_start, styles_css_end - styles_css_start);
	}
	else if (strcmp(req->uri, http_common_js_url) == 0)
	{
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_JS);
		httpd_resp_send(req, (char *)common_js_start, common_js_end - common_js_start);
	}
	else if (strcmp(req->uri, http_logo_url) == 0)
	{
		ESP_LOGD(TAG, "Sending logo.");
		httpd_resp_set_status(req, HTTPD_200);
		httpd_resp_set_type(req, contentType_png);
		httpd_resp_send(req, (char *)logo_start, logo_end - logo_start);
		return ESP_OK;
	}
	else if (strcmp(req->uri, http_get_networks_url) == 0)
	{
		char *aps_list = get_available_networks_list();
		if (aps_list == NULL)
		{
			httpd_resp_set_status(req, HTTPD_204);
			httpd_resp_send(req, NULL, 0);
			return ESP_OK;
		}

		ESP_LOGD(TAG, "APs List: %s", aps_list);

		httpd_resp_set_status(req, HTTPD_200);
		httpd_resp_set_type(req, HTTPD_TYPE_JSON);
		httpd_resp_send(req, aps_list, strlen(aps_list));

		if (aps_list)
		{
			ESP_LOGI(TAG, "APs List size: %d", (int)strlen(aps_list));
			free(aps_list), aps_list = NULL;
		}
		else
		{
			ESP_LOGI(TAG, "aps_list already NULL");
		}
		return ESP_OK;
	}

	httpd_resp_set_status(req, http_404_hdr);
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}
