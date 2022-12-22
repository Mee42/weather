#pragma once

#include <stdbool.h>

typedef struct {
	int start; // hour
	int end; // hour
	bool is_snow;
} weather_t;

typedef struct {
	char day[2];
	int high;
	int low;
	weather_t weather[2];
} day_t;

typedef enum {
	SS_STARTING,
	SS_FORCAST,
} screenstate_t;



void init_screen(void);

void render_forcast_menu(day_t* day);
