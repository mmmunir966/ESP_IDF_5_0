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
char *redirectURL = NULL;
static const char http_200_hdr[] = "200 OK";
static const char http_404_hdr[] = "404 Not Found";
static const char contentType_png[] = "image/png";
static const char http_cache_control_hdr[] = "Cache-Control";
static const char http_cache_control_no_cache[] = "no-store, no-cache, must-revalidate, max-age=0";
static const char http_pragma[] = "Pragma";
static const char http_pragma_no_cache[] = "no-cache";

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
		if (redirectURL)
			free(redirectURL), redirectURL = NULL;

		/* redirect url */
		size_t redirect_sz = strlen("http://255.255.255.255/") + 1;
		redirectURL = (char *)malloc(sizeof(char) * redirect_sz);
		*redirectURL = '\0';

		snprintf(redirectURL, redirect_sz, "http://%s/", DEFAULT_AP_IP);

		httpd_config_t config = HTTPD_DEFAULT_CONFIG();

		config.uri_match_fn = httpd_uri_match_wildcard;
		config.lru_purge_enable = true;

		if (httpd_start(&httpd_handle, &config) == ESP_OK)
		{
			ESP_LOGI(TAG, "Registering URI handlers");
			httpd_register_uri_handler(httpd_handle, &httpd_register_get_request);
			httpd_register_uri_handler(httpd_handle, &httpd_register_post_request);
		}
		dns_start();
	}
}

void stop_web_server()
{
	if (httpd_handle != NULL)
	{
		httpd_stop(httpd_handle), httpd_handle = NULL;
		dns_stop();
	}
}

static esp_err_t http_post_request_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "POST %s", req->uri);

	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
	httpd_resp_set_hdr(req, "Access-Control-Max-Age", "3600");
	httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
	httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);

	if (strcmp(req->uri, http_update_wifi_cr_url) == 0)
	{
		char *ssid = NULL;
		char *password = NULL;
		size_t ssid_len = 0, password_len = 0;

		ssid_len = httpd_req_get_hdr_value_len(req, WIFI_SSID_KEY);
		password_len = httpd_req_get_hdr_value_len(req, WIFI_PASSWORD_KEY);

		ESP_LOGD(TAG, "Updating wifi credentials with ssid_len %d and password_len %d.", ssid_len, password_len);

		if (ssid_len && ssid_len <= MAX_SSID_LEN)
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

			if (ssid)
			{
				free(ssid), ssid = NULL;
			}

			if (password)
			{
				free(password), password = NULL;
			}
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
	httpd_resp_set_hdr(req, "Access-Control-Max-Age", "3600");

	if (strcmp(req->uri, http_index_html_url) == 0)
	{
		ESP_LOGI(TAG, "Sending index.htm file.");
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
		httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
		httpd_resp_send(req, (char *)index_html_start, index_html_end - index_html_start);
		return ESP_OK;
	}
	else if (strcmp(req->uri, http_styles_css_url) == 0)
	{
		ESP_LOGI(TAG, "Sending styles.css file.");
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
		httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_CSS);
		httpd_resp_send(req, (char *)styles_css_start, styles_css_end - styles_css_start);
		return ESP_OK;
	}
	else if (strcmp(req->uri, http_common_js_url) == 0)
	{
		ESP_LOGI(TAG, "Sending common.js file.");
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
		httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, HTTPD_TYPE_JS);
		httpd_resp_send(req, (char *)common_js_start, common_js_end - common_js_start);
		return ESP_OK;
	}
	else if (strcmp(req->uri, http_logo_url) == 0)
	{
		ESP_LOGI(TAG, "Sending logo.png file.");
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
		httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);
		httpd_resp_set_status(req, HTTPD_200);
		httpd_resp_set_type(req, contentType_png);
		httpd_resp_send(req, (char *)logo_start, logo_end - logo_start);
		return ESP_OK;
	}
	else if (strcmp(req->uri, http_get_networks_url) == 0)
	{
		ESP_LOGI(TAG, "Sending networks list.");
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
		httpd_resp_set_hdr(req, http_pragma, http_pragma_no_cache);
		char *aps_list = get_available_networks_list();
		if (aps_list == NULL)
		{
			httpd_resp_set_status(req, HTTPD_204);
			httpd_resp_send(req, "{}", strlen("{}"));
			return ESP_OK;
		}

		ESP_LOGD(TAG, "APs List: %s", aps_list);

		httpd_resp_set_status(req, HTTPD_200);
		httpd_resp_set_type(req, HTTPD_TYPE_JSON);
		httpd_resp_send(req, aps_list, strlen(aps_list));
		return ESP_OK;
	}

	// Captive portal redirect.
	ESP_LOGW(TAG, "Redirecting request to %s.", redirectURL);
	httpd_resp_set_status(req, "302 Found");
	httpd_resp_set_hdr(req, "Location", redirectURL);
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}
