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


#define MAX_HTTP_RECV_BUFFER 512
// an absolutely absurd buffer size 
#define MAX_HTTP_OUTPUT_BUFFER (64 * 1024)

#include <time.h>

// private functions
cJSON* find_field(cJSON* first_child, char* name, bool warnOnMissing);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);


// where the api text gets put
char *local_response_buffer; 
// where we store the api data
forcast_data data_buf[40];

void init_http() {
	local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
}


void make_api_call() {
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
}

void parse_forcast_data() {
	cJSON *json = cJSON_Parse(local_response_buffer);
	if(json == NULL) {
		ESP_LOGE(TAG, "json is null");
		return;
	}
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


	setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
	tzset();
	// but we'll just use the first element for now
	for(int i = 0; i < 40; i++) {

		cJSON *obj = json->child; // this is the first field of the inner obj
		// extract the dt
		cJSON *dt = find_field(obj, "dt", true);
		if(dt == NULL) return;
		if(dt->type != cJSON_Number) {
			ESP_LOGE(TAG, "was expecting a number at root['list'][i]['dt']");
			return;
		}
	
		time_t dt_time_t = (time_t)(dt->valuedouble);
		struct tm * const time = localtime(&dt_time_t);
		data_buf[i].hour = time->tm_hour;
		data_buf[i].day_of_week = time->tm_wday;
		
		cJSON *main = find_field(obj, "main", true);
		if(main == NULL) return;
		if(main->type != cJSON_Object) {
			ESP_LOGE(TAG, "was expecting an object at root['list'][i]['main']");
			return;
		}
		main = main->child; // first field
		cJSON *temp_min 	= find_field(main, "temp_min", true);
		cJSON *temp_max 	= find_field(main, "temp_max", true);
		cJSON *temp 		= find_field(main, "temp", true);
		cJSON *feels_like 	= find_field(main, "feels_like", true);
		if(temp_min == NULL || temp_max == NULL || 
		   temp == NULL     || feels_like == NULL) {
			return;
		}
		// we're just ging to assume they are the right type lol
		data_buf[i].temp 	 = temp->valuedouble;
		data_buf[i].temp_max = temp_max->valuedouble;
		data_buf[i].temp_min = temp_min->valuedouble;
		data_buf[i].feels_like = feels_like->valuedouble;

		cJSON *pop = find_field(obj, "pop", true);
		if(pop == NULL) return;
		data_buf[i].percipitation_probability = pop->valuedouble;

		cJSON *cloudyness = find_field(find_field(obj, "clouds", true)->child,
								       "all", true);
		if(cloudyness == NULL) return;
		data_buf[i].cloudyness = cloudyness->valueint;


		cJSON *rain = find_field(obj, "rain", false);
		if(rain != NULL) {
			rain = find_field(rain->child, "3h", true);
			if(rain == NULL) return;
			data_buf[i].rain_volume = rain->valuedouble;
		} else {
			data_buf[i].rain_volume = 0;
		}


		cJSON *snow = find_field(obj, "snow", false);
		if(snow != NULL) {
			snow = find_field(snow->child, "3h", true);
			if(snow == NULL) return;
			data_buf[i].snow_volume = snow->valuedouble;
		} else {
			data_buf[i].snow_volume = 0;
		}


		// go to the next element
		json = json->next;

	}
	for(int i = 0; i < 40; i++) {
		forcast_data f = data_buf[i];
		ESP_LOGI(TAG, "looking at record at %d (%d:00)\n"
				"feels_like: %f. temp_min: %f. temp_max: %f. temp: %f\n"
				"percip: %f. cloudyness: %d. rain vol: %f. snow vol: %f", 
				f.day_of_week, f.hour,
				f.feels_like, f.temp_min, f.temp_max, f.temp,
				f.percipitation_probability, f.cloudyness,
				f.rain_volume, f.snow_volume);
	}	
}

cJSON* find_field(cJSON* first_child, char* name, bool warnOnMissing) {
	if(first_child == NULL) return NULL;
	cJSON* obj = first_child;	
	while(strcmp(obj->string, name) != 0) {
		obj = obj->next;
		if(obj == NULL) {
			if(warnOnMissing) {
				ESP_LOGE(TAG, "reached end of object while looking for %s", name);
			}
			return NULL;
		}
	}
	return obj;
}

int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return b > a ? a : b; }

// just all of the days in order lol
char *day_lookup = "Su" "Mo" "Tu" "We" "Th" "Fr" "Sa";

// ret must be an array with 7 elements
int process_data(day_t *ret) {
	for(int i = 0; i <= 6; i++) {
		day_t v = {
			.day = { day_lookup[i * 2], day_lookup[i * 2 + 1] },
			.low = 99,
			.high = -9,
			.weather = {
				{ .start = -99, .end = -99, .is_snow = false },
				{ .start = -99, .end = -99, .is_snow = false }
			}
		};
		ret[i] = v;
	}
	for(int i = 0; i < 40; i++) {
		forcast_data f = data_buf[i];
		day_t *ptr = ret + f.day_of_week;
		ptr->high = max(ptr->high, f.temp_max);
		ptr->low = min(ptr->low, f.temp_min);
		
		if(f.rain_volume != 0) {
			// find a spot we can put the weather data lol
			for(int w = 0; w < 2; w++) {
				weather_t *weather = &ptr->weather[w];
				if(weather->start == -99) {
					ESP_LOGI(TAG, "(w = %d) making new weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					weather->end = f.hour;
					weather->start = max(f.hour - 3, 0); // zero is bottom
					weather->is_snow = false;
					break;
				}
				// if it's already been claimed, check to make sure it isn't for snow
				if(weather->is_snow) break;
				// if we are after the current recorded segment
				if(weather->end == f.hour - 3) {
					weather->end = f.hour;
					ESP_LOGI(TAG, "(w = %d) prepending weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					break;
				}
				if(weather->start == f.hour) {
					ESP_LOGI(TAG, "(w = %d) appending weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					weather->start = max(f.hour - 3, 0);
					break;
				}
			}
		}
		if(f.snow_volume != 0) {
			// find a spot we can put the weather data lol
			for(int w = 0; w < 2; w++) {
				weather_t *weather = &ptr->weather[w];
				if(weather->start == -99) {
					ESP_LOGI(TAG, "(w = %d) making new weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					weather->end = f.hour;
					weather->start = max(f.hour - 3, 0); // zero is bottom
					weather->is_snow = true;
					break;
				}
				// if it's already been claimed, check to make sure it isn't for rain
				if(!weather->is_snow) break;
				// if we are after the current recorded segment
				if(weather->end == f.hour - 3) {
					weather->end = f.hour;
					ESP_LOGI(TAG, "(w = %d) prepending weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					break;
				}
				if(weather->start == f.hour) {
					ESP_LOGI(TAG, "(w = %d) appending weather %d %d-%d",
							w, f.day_of_week,
							max(f.hour - 3, 0), f.hour);
					weather->start = max(f.hour - 3, 0);
					break;
				}
			}
		}
	}
	return data_buf[0].day_of_week;
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

