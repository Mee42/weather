#include <stdint.h>
#include <stdio.h>
#include "http.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wpa2.h"
#include "rom/ets_sys.h"
#include "esp_timer.h"

#include <inttypes.h>

#include "main.h"
#include "wifi.h"
#include "screen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"



#define TAG "main.c"


bool getButton() {
	return gpio_get_level(GPIO_NUM_18);
}



void init_nvs_flash() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

day_t days[7];

void app_main(void)
{
	ESP_LOGI(TAG,"setting top button gpio as input");
	// this is the top button
	gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1 << GPIO_NUM_18;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

	ESP_LOGI(TAG, "setting up nvs flash");
	init_nvs_flash();

	ESP_LOGI(TAG, "setting up screen...");
	init_screen();

	ESP_LOGI(TAG, "calling wifi_init_sta");	
	wifi_init_sta();

	ESP_LOGI(TAG, "setting up http");
	init_http();	
	
	ESP_LOGI(TAG, "making api call");
	make_api_call();
	
	ESP_LOGI(TAG, "parsing forcast data");
	parse_forcast_data();
	
	ESP_LOGI(TAG, "processing data");
	int current_day = process_data(days);
	
	int64_t last_data_refresh_time = esp_timer_get_time();
	int64_t time_between_updates = 1 /*min*/ * 60 * 1000 * 1000; // in microseconds

	int i = current_day;
	while(true) {
		while(days[i %= 7].low == 99) { i++; } // find next day
		render_forcast_menu(&days[i++]);
		// we wait for the button to be unpressed before waiting for it to be pressed
		while(getButton()) { vTaskDelay(1); }; 
		int64_t press_time = esp_timer_get_time();
		int spinlock = 0;
		while(!getButton()) {
			vTaskDelay(10 / portTICK_PERIOD_MS);
			if(spinlock++ == 100) {
				spinlock = 0;
				int64_t now_time = esp_timer_get_time();
				if((now_time - press_time) / 1000 / 1000 > 30) {
					i = current_day;
					break;
				}
				if(now_time > last_data_refresh_time + time_between_updates) {
					ESP_LOGI(TAG, "refetching data!");
					last_data_refresh_time = now_time;
					make_api_call();
					parse_forcast_data();
					process_data(days);
					i = current_day;
					break;
				}
			}
		}
	}

	ESP_LOGI("==================", "Reached the end of program execution. Halting");
	while(true) vTaskDelay(1000);
}




