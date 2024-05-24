#include "location.h"
#include "requests.h"
#include <stdlib.h>
#include <string.h>

const static char *LOCATION_API = "http://ip-api.com/json/?fields=lat,lon,timezone,offset,country,city";

location_t location;

void location_json_parser(cJSON *root) {
  location.lat = cJSON_GetObjectItem(root, "lat")->valuedouble;
  location.lon = cJSON_GetObjectItem(root, "lon")->valuedouble;
  // location.offset = cJSON_GetObjectItem(root, "offset")->valueint;

  free(location.timezone);
  free(location.country);
  free(location.city);

  location.timezone = strdup(cJSON_GetObjectItem(root, "timezone")->valuestring);
  location.country = strdup(cJSON_GetObjectItem(root, "country")->valuestring);
  location.city = strdup(cJSON_GetObjectItem(root, "city")->valuestring);
}

esp_err_t location_request() { return requests_get_json(LOCATION_API, location_json_parser); }

location_t *location_get() { return &location; }
