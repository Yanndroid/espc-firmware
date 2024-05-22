#pragma once

#include "weather.h"
#include "ws2812b.h"
#include <stdbool.h>
#include <sys/time.h>

void display_init(void);

void display_set_brightness(uint8_t brightness, bool update);

void display_show_boot(void);

void display_show_app(void);

void display_show_test(uint8_t a, uint8_t b, uint8_t c, uint8_t d, color_t color);

void display_show_time(struct tm *timeinfo, color_t color);

void display_show_date(struct tm *timeinfo, color_t day_color, color_t mon_color);

void display_show_temperature_degree(weather_t *weather, color_t color);

void display_show_temperature_weather(weather_t *weather, color_t color);

void display_show_loading_next();