#pragma once

#include <esp_err.h>

typedef struct location_t {
  double lat;
  double lon;
  // int offset;
  char *timezone;
  char *country;
  char *city;
} location_t;

esp_err_t location_request();

location_t *location_get();