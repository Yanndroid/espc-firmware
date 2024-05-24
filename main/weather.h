#pragma once

#include "location.h"

typedef struct weather_t {
  int8_t temperature;
  int8_t feels_like;
  uint8_t weather_code;
  time_t sunrise;
  time_t sunset;
  time_t next_update;
} weather_t;

esp_err_t weather_request(location_t *location);

weather_t *weather_get();