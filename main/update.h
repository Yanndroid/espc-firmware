#pragma once

#include "esp_err.h"
#include "esp_http_client.h"

typedef struct {
    char *url;
    char *signature;
    http_event_handle_cb event_handle;
} update_data_t;

esp_err_t update_ota(update_data_t *data);