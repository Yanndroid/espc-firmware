#pragma once

#include <stdbool.h>
#include <sys/_stdint.h>

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

settings_t settings;

void storage_init(void);

void storage_wipe(void);

void storage_save_wifi(void);

void storage_save_brightness(void);

void storage_save_color(void);