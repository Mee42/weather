#pragma once

#include "screen.h"


void init_http();
void make_api_call();
void parse_forcast_data();
// returns what day is today
int process_data(day_t *ret);


typedef struct {
	int day_of_week; // this is the day of the week
	int hour; // 15 -> 15:00 -> 3pm local time
	double feels_like, temp_min, temp_max, temp; // farenhite
	double percipitation_probability; // 0..1
	int cloudyness;  // [0, 100] percentage
	double rain_volume; // mm
	double snow_volume; // mm
} forcast_data;

