#include "screen.h"

#include <stdio.h>
#include <string.h>

#include "ssd1306.h"
#include "font8x8_basic.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define tag "screen.c"

// private functions



void display_str_x2(int seg, int page,
		            char* str, size_t len, bool invert);

void display_str_x1(int seg, int page,
				    char* str, size_t len, bool invert);

// TODO seg must = 0
void display_str_x3(int seg, int page,
				    char* str, size_t len, bool invert);

SSD1306_t dev;

screenstate_t current_mode;


void render_forcast_menu(day_t *info) {
	ESP_LOGI("render_forcast_menu", "rendering forcast menu");

	if(current_mode != SS_FORCAST) {
		// fill in the 'b' section LOL
		display_str_x1(0, 0, "      ", 6, true);
		for(int i = 0; i < 4; i++) {
			display_str_x1(6 * 8, i, " ", 1, false);
			display_str_x1(15 * 8, i, " ", 1, false);
		}
	}

	display_str_x3(0, 1, info->day, 2, true);

	char buf[5];
	sprintf(buf, "L:%2d", info->low);
	display_str_x2(7 * 8, 0, buf, strlen(buf), false); 
	sprintf(buf, "H:%2d", info->high);
	display_str_x2(7 * 8, 2, buf, strlen(buf), false); 

	display_str_x2(0, 4, "01234567", 8, false);
	display_str_x2(0, 6, "01234567", 8, false);

	current_mode = SS_FORCAST;
}
//    ----------------
//  | bbbbbb YYYYYYYY
//  | XXXXXX YYYYYYYY
//  | XXXXXX ZZZZZZZZ
//  | XXXXXX ZZZZZZZZ
//  | wwwwwwwwwwwwwww
//  | wwwwwwwwwwwwwww
//  | mmmmmmmmmmmmmmm
//  | mmmmmmmmmmmmmmm
//
//
//  XX - the specifici day
//  YY - the low
//  ZZ - the high
//  www, mmm -> two lines of 2x text for rain/snow

void display_str_x2(int seg, int page,
		            char* str, size_t len, bool invert) {
	if (page >= dev._pages) return;
	if (len > 16) len = 8;
	uint8_t buf[16]; // we're writing 16 pages
	for(int str_i = 0; str_i < len; str_i++){
		uint8_t const * const inCols = font8x8_basic_tr[(uint8_t)(str[str_i])];
		for(int pageI = 0; pageI <= 1; pageI++) {
			for(int x = 0; x < 16; x++) {
				// x is the column index
				buf[x] = 0;
				for(int y = 0; y < 8; y++){
					// y is the position of the bit that we are writing
					int isOn = 0 != (inCols[x/2] & (1 << (y/2 + pageI * 4)));
					if(isOn) buf[x] |= 1 << y;
				}
			}
			if (invert) ssd1306_invert(buf, 8);
			i2c_display_image(&dev, pageI + page, seg, buf, 16);
		}
		seg += 16;
	}
}
void display_str_x1(int seg, int page,
				    char* str, size_t len, bool invert) {
	if (page >= dev._pages) return;
	if (len > 16) len = 16;

	uint8_t buf[8];
	for (uint8_t i = 0; i < len; i++) {
		memcpy(buf, font8x8_basic_tr[(uint8_t)str[i]], 8);
		if (invert) ssd1306_invert(buf, 8);
		ssd1306_display_image(&dev, page, seg, buf, 8);
		seg = seg + 8;
	}
}
void display_str_x3(int seg, int page,
					char* str, size_t len, bool invert) {
	ssd1306_display_text_x3(&dev, page, str, len, invert);
}



void init_screen(void) {
	ESP_LOGI(tag, "starting i2c");
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

	ESP_LOGI(tag, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);

	ssd1306_clear_screen(&dev, true);

	ssd1306_display_text(&dev, 3, "             ", strlen("             "), false);
	ssd1306_display_text(&dev, 4, "    starting ", strlen("             "), false);
	ssd1306_display_text(&dev, 5, "             ", strlen("             "), false);
	ssd1306_display_text(&dev, 3, "   ", strlen("   "), true);
	ssd1306_display_text(&dev, 4, "   ", strlen("   "), true);
	ssd1306_display_text(&dev, 5, "   ", strlen("   "), true);
	

	current_mode = SS_STARTING;
}














// this is only for reference
void other_shit() {
	int center, top, bottom;
	char lineChar[20];
	int i = 0;
	while(true) {
		//render_forcast_menu(&dev, &days[i++ % 5]);
		break;
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
	//	while(getButton());
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
	
}
