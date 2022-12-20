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

#include "ssd1306.h"
#include "font8x8_basic.h"


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

day_t days[5];

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
	
	ESP_LOGI(TAG, "CALLING HTTP FUNCTION");
	get_forcast_temp();

	int i = 0;
	while(true) {
		render_forcast_menu(&days[i++ % 5]);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI("==================", "Reached the end of program execution. Halting");
	while(true) vTaskDelay(1000);
}




day_t days[5] = {
	{
		.day = { 'T', 'u' },
		.high = 35,
		.low = 22
	},
	{
		.day = {'W', 'e'},
		.high = 58,
		.low = 34
	},
	{
		.day = { 'T', 'h' },
		.high = 92,
		.low = 4
	},
	{
		.day = {'F', 'r'},
		.high = 13,
		.low = -4
	},
	{
		.day = {'S', 'a'},
		.high = 99,
		.low = 77
	},
};
