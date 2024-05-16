#pragma once

#include <cJSON.h>
#include <esp_err.h>

typedef void (*requests_json_parser)(cJSON *root);

esp_err_t requests_get_json(const char *url, requests_json_parser parser);