#include "http.h"

#include <stdio.h>
#include <string.h>
//#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#define TAG "http.c"

#include "json/cJSON.h"

#include "../secrets.h"

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

#define MAX_HTTP_RECV_BUFFER 512
// an absolutely absurd buffer size 
#define MAX_HTTP_OUTPUT_BUFFER (64 * 1024)

#include <time.h>


char *local_response_buffer; 

void init_http() {
	local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
}


typedef struct {
	int day_of_week; // this is the day of the week
	int hour; // 15 -> 15:00 -> 3pm local time
	double feels_like, temp_min, temp_max, temp; // farenhite
	double percipitation_probability; // 0..1
	double cloudyness; // I've seen 2. is a %. out of 100? not sure
	double rain_volume; // mm
	double snow_volume; // mm
} forcast_data;


void get_forcast_temp() {
    esp_http_client_config_t config = {
		.url = API_INCLU_PASS,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
	ESP_LOGI(TAG, "INITTED HTTP ALLEGEDLY");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		return;
    }
	if(esp_http_client_get_status_code(client) != 200) {
		ESP_LOGE(TAG, "HTTP REQ FAILED: %d", esp_http_client_get_status_code(client));
		return;
	}


	cJSON *json = cJSON_Parse(local_response_buffer);
	if(json->type != cJSON_Object) {
		ESP_LOGE(TAG, "was expecting object at top level");
		return;
	}
	json = json->child;
	while(strcmp(json->string, "list") != 0) {
		json = json->next;
		if(json == NULL) {
			ESP_LOGE(TAG, "reached end of object while looking for dt");
			return;
		}
	}
	if(json->type != cJSON_Array) {
		ESP_LOGE(TAG, "was expecting array at root['list']");
		return;
	}
	json = json->child; // the first element
	if(json->type != cJSON_Object) {
		ESP_LOGE(TAG, "was expecting an object at root['list'][0]");
		return;
	}

	//forcast_data data[40] = { 0 };

	setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
	tzset();
	// but we'll just use the first element for now
	for(int i = 0; i < 40; i++) {

		cJSON *obj = json->child; // this is the first field of the inner obj
		// extract the dt
		while(strcmp(obj->string, "dt") != 0) {
			obj = obj->next;
			if(obj == NULL) {
				ESP_LOGE(TAG, "reached end of object while looking for dt");
				return;
			}
		}
		if(obj->type != cJSON_Number) {
			ESP_LOGE(TAG, "was expecting an object at root['list'][0]['dt']");
			return;
		}
	
		time_t dt = (time_t)(obj->valuedouble);
		ESP_LOGI("=====", "time_t dt: %d", (double)dt);
		struct tm * const time = localtime(&dt);
		//data[i].hour = x;
		ESP_LOGI("======", "GOT DT_0: %s", asctime(time));
		json = json->next;
	}
	
}




esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

