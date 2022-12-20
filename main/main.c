#include <stdint.h>
#include <stdio.h>
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


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"


#define tag "SSD1306"

bool getButton() {
	return gpio_get_level(GPIO_NUM_18);
}

typedef struct {
	char day[2];
	int high;
	int low;
} day_t;

void display_str_x2(SSD1306_t*,int,int,char*,size_t,bool);

void render_forcast_menu(SSD1306_t *dev, day_t *info) {
	ESP_LOGI("main:render_forcast_menu", "swapped day");
	//ssd1306_clear_screen(dev, false);
//	ssd1306_display_text_x3(dev, 0, "  XXX", 5, false);
	ssd1306_display_text(dev, 0, "      ", 6, true);
	ssd1306_display_text_x3(dev, 1, info->day, 2, true);
	char buf[50];
	sprintf(buf, "L:%2d", info->low);
	display_str_x2(dev, 7 * 8, 0, buf, strlen(buf), false); 
	sprintf(buf, "H:%2d", info->high);
	display_str_x2(dev, 7 * 8, 2, buf, strlen(buf), false); 
	display_str_x2(dev, 0, 4, "01234567", 8, false);
	display_str_x2(dev, 0, 6, "01234567", 8, false);
}
//    ----------------
//  | bbbbbb YYYYYYYY
//  | XXXXXX YYYYYYYY
//  | XXXXXX ZZZZZZZZ
//  | XXXXXX ZZZZZZZZ
//  | xxyyxxyyxxyyxxyy
//  | xxyyxxyyxxyyxxyy
//  |
//  |

void display_str_x2(SSD1306_t *dev, int seg, int pageOffset,
		            char* str, size_t len, bool inverted) {
	uint8_t buf[16]; // we're writing 16 pages
	for(int str_i = 0; str_i < len; str_i++){
		uint8_t const * const inCols = font8x8_basic_tr[(uint8_t)(str[str_i])];
		for(int page = 0; page <= 1; page++) {
			for(int x = 0; x < 16; x++) {
				// x is the column index
				buf[x] = 0;
				for(int y = 0; y < 8; y++){
					// y is the position of the bit that we are writing
					int isOn = 0 != (inCols[x/2] & (1 << (y/2 + page * 4)));
					if(isOn) buf[x] |= 1 << y;
				}
			}
			i2c_display_image(dev, page + pageOffset, seg, buf, 16);
		}
		seg += 16;
	}
}



void init_nvs_flash() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
	ESP_LOGI("app_main","setting top button gpio as input");
	// this is the top button
	gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1 << GPIO_NUM_18;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

	ESP_LOGI("app_main", "setting up nvs flash");
	init_nvs_flash();

	ESP_LOGI("app_main", "calling wifi_init_sta");	
	wifi_init_sta();



	ESP_LOGI("app_main", "setting up screen...");

	SSD1306_t dev;
	int center, top, bottom;
	char lineChar[20];

	ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

	ESP_LOGI(tag, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);

	ssd1306_clear_screen(&dev, false);

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
	int i = 0;
	while(true) {
		render_forcast_menu(&dev, &days[i++ % 5]);
		while(!getButton()) vTaskDelay(1);
		while(getButton())  vTaskDelay(1);
	}

	while(true) vTaskDelay(30000 / portTICK_PERIOD_MS);
		
	char buf[20];
	while(true) {
  		//vTaskDelay(3 / portTICK_PERIOD_MS);
//		if(getButton()) {
//  			ssd1306_display_text_x3(&dev, 1, "PRESSED", 5, true);
//		} else {
//			ssd1306_display_text_x3(&dev, 1, "RELES.", 5, true); 
//		}
		while(getButton());
		sprintf(buf, "%i", ++i);
  		ssd1306_display_text(&dev, 1, buf, strlen(buf), true);
	}
	top = 2;
	center = 3;
	bottom = 8;
	ssd1306_display_text(&dev, 0, "SSD1306 128x64", 14, false);
	ssd1306_display_text(&dev, 1, "ABCDEFGHIJKLMNOP", 16, false);
	ssd1306_display_text(&dev, 2, "abcdefghijklmnop",16, false);
	ssd1306_display_text(&dev, 3, "Hello World!!", 13, false);
	//ssd1306_clear_line(&dev, 4, true);
	//ssd1306_clear_line(&dev, 5, true);
	//ssd1306_clear_line(&dev, 6, true);
	//ssd1306_clear_line(&dev, 7, true);
	ssd1306_display_text(&dev, 4, "SSD1306 128x64", 14, true);
	ssd1306_display_text(&dev, 5, "ABCDEFGHIJKLMNOP", 16, true);
	ssd1306_display_text(&dev, 6, "abcdefghijklmnop",16, true);
	ssd1306_display_text(&dev, 7, "Hello World!!", 13, true);

	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
	// Display Count Down
	uint8_t image[24];
	memset(image, 0, sizeof(image));
	ssd1306_display_image(&dev, top, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&dev, top+1, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&dev, top+2, (6*8-1), image, sizeof(image));
	for(int font=0x39;font>0x30;font--) {
		memset(image, 0, sizeof(image));
		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
		memcpy(image, font8x8_basic_tr[font], 8);
		if (dev._flip) ssd1306_flip(image, 8);
		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	
	// Scroll Up
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "---Scroll  UP---", 16, true);
	//ssd1306_software_scroll(&dev, 7, 1);
	ssd1306_software_scroll(&dev, (dev._pages - 1), 1);
	for (int line=0;line<bottom+10;line++) {
		lineChar[0] = 0x01;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
	// Scroll Down
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "--Scroll  DOWN--", 16, true);
	//ssd1306_software_scroll(&dev, 1, 7);
	ssd1306_software_scroll(&dev, 1, (dev._pages - 1) );
	for (int line=0;line<bottom+10;line++) {
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Page Down
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "---Page	DOWN---", 16, true);
	ssd1306_software_scroll(&dev, 1, (dev._pages-1) );
	for (int line=0;line<bottom+10;line++) {
		//if ( (line % 7) == 0) ssd1306_scroll_clear(&dev);
		if ( (line % (dev._pages-1)) == 0) ssd1306_scroll_clear(&dev);
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Horizontal Scroll
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "Horizontal", 10, false);
	ssd1306_hardware_scroll(&dev, SCROLL_RIGHT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_LEFT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
	
	// Vertical Scroll
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "Vertical", 8, false);
	ssd1306_hardware_scroll(&dev, SCROLL_DOWN);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_UP);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
	
	// Invert
	ssd1306_clear_screen(&dev, true);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "  Good Bye!!", 12, true);
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	// Fade Out
	ssd1306_fadeout(&dev);
	
	// Restart module
	esp_restart();
}

