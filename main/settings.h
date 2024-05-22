#pragma once

#include "sdkconfig.h"
#include <stdbool.h>
#include <sys/_stdint.h>

// to save: name, wifi creds, brightness config, color settings, custom loction

typedef struct {
  char ssid[32];
  char password[64];
  char username[32];
} settings_wifi_t;

typedef struct {
  uint8_t min;
  uint8_t max;
  uint16_t margin;
} settings_brightness_t;

typedef struct {
  uint16_t hue;
  uint8_t sat;
} settings_color_t;

typedef struct {
  char device_name[32];
  settings_wifi_t wifi;
  settings_brightness_t brightness;
  settings_color_t color;
} settings_t;

settings_t settings = {
    .device_name = CONFIG_DEVICE_NAME,
    .wifi =
        {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .username = CONFIG_WIFI_USERNAME,
        },
    .brightness =
        {
            .min = CONFIG_BRIGHTNESS_MIN,
            .max = CONFIG_BRIGHTNESS_MAX,
            .margin = CONFIG_BRIGHTNESS_MARGIN,
        },
    .color =
        {
            .hue = CONFIG_COLOR_HUE,
            .sat = CONFIG_COLOR_SAT,
        },
};
