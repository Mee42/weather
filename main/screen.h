#pragma once


typedef struct {
	char day[2];
	int high;
	int low;
} day_t;

typedef enum {
	SS_STARTING,
	SS_FORCAST,
} screenstate_t;



void init_screen(void);

void render_forcast_menu(day_t* day);
