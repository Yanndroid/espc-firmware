#include "weather.h"
#include "requests.h"
#include <math.h>

const static char *WEATHER_API = "http://api.open-meteo.com/v1/forecast?"
                                 "current=temperature,apparent_temperature,weather_code"
                                 "&daily=sunrise,sunset"
                                 "&timeformat=unixtime"
                                 "&forecast_days=1"
                                 "&timezone=%s"
                                 "&latitude=%.4f"
                                 "&longitude=%.4f";

weather_t weather;

void weather_json_parser(cJSON *root) {
  cJSON *current = cJSON_GetObjectItem(root, "current");
  cJSON *daily = cJSON_GetObjectItem(root, "daily");

  if (!current || !daily)
    return;

  weather.temperature = round(cJSON_GetObjectItem(current, "temperature")->valuedouble);
  weather.feels_like = round(cJSON_GetObjectItem(current, "apparent_temperature")->valuedouble);
  weather.weather_code = cJSON_GetObjectItem(current, "weather_code")->valueint;

  weather.sunrise = cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunrise"), 0)->valueint;
  weather.sunset = cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunset"), 0)->valueint;

  weather.next_update = cJSON_GetObjectItem(current, "time")->valueint + cJSON_GetObjectItem(current, "interval")->valueint;
}

esp_err_t weather_request(location_t *location) {
  char url[1 + snprintf(NULL, 0, WEATHER_API, location->timezone, location->lat, location->lon)];
  snprintf(url, sizeof(url), WEATHER_API, location->timezone, location->lat, location->lon);

  return requests_get_json(url, weather_json_parser);
}

weather_t *weather_get() { return &weather; }