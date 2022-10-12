#include "http.h"
#include "main.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <stdlib.h>

extern uint8_t index_html_start[] asm("_binary_index_html_start"); 
extern uint8_t index_html_end[]   asm("_binary_index_html_end"); 

static esp_err_t get_handler(httpd_req_t* req) {
    httpd_resp_send(req, (char*)index_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static char* get_header_and_malloc(httpd_req_t* req, const char* name) {
	int len = httpd_req_get_hdr_value_len(req, name);
	char* buf = calloc(1, len + 2);
	httpd_req_get_hdr_value_str(req, name, buf, len + 1);
	return buf;
}


static esp_err_t spin_handler(httpd_req_t* req) {
    const char resp[] = "";

	char* a_ticks_str = get_header_and_malloc(req, "_wb_a_ticks");
	int a_ticks = atoi(a_ticks_str);
	free(a_ticks_str);
	
	char* b_ticks_str = get_header_and_malloc(req, "_wb_b_ticks");
	int b_ticks = atoi(b_ticks_str);
	free(b_ticks_str);
	
	char* a_hz_str = get_header_and_malloc(req, "_wb_a_hz");
	int a_hz = atoi(a_hz_str);
	free(a_hz_str);
	
	char* b_hz_str = get_header_and_malloc(req, "_wb_b_hz");
	int b_hz = atoi(b_hz_str);
	free(b_hz_str);
	

	ESP_LOGI("http", "a: %d ticks, %d hz, b: %d ticks, %d hz", 
			a_ticks, a_hz, b_ticks, b_hz);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	spin_stepper(a_ticks, b_ticks, a_hz, b_hz);
    return ESP_OK;
} 

static const httpd_uri_t uri_get_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_spin = {
    .uri = "/spin",
    .method = HTTP_POST,
    .handler = spin_handler,
};

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI("http", "Starting server on port: %d", config.server_port); 
    httpd_handle_t server = NULL;
    if(httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get_root);
        httpd_register_uri_handler(server, &uri_post_spin);
    }
    return server; // can be null
}
