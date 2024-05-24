#include "settings.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <stdlib.h>
#include <string.h>

#define SETTINGS_STORAGE_NAMESPACE "settings"

//            ssid | creds | name | bri conf | clr conf | locaction | dot blink | bri cycle |
// coap get:   x   |       |   x  |    x     |    x     |     x     |     x     |     x     |
// coap put:   /   |   /   |   x  |    x     |    x     |           |     x     |     x     | + reboot, reset
// settings:   x   |   x   |   x  |    x     |    x     |           |     x     |     x     |
// nvs:        x   |   x   |   x  |    x     |    x     |           |     x     |     x     |

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

static void storage_load_blob(nvs_handle handle, const char *key, void *value, size_t size) {
  esp_err_t err = nvs_get_blob(handle, key, value, &size);
  if (err != ESP_OK) {
    ESP_LOGE("storage", "load: %s: %s", key, esp_err_to_name(err));
  }
}

static void storage_save_blob(const char *key, void *value, size_t size) {
  nvs_handle handle;
  esp_err_t err = nvs_open(SETTINGS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE("storage", "open: %s", esp_err_to_name(err));
    return;
  }

  void *current = malloc(size);
  err = nvs_get_blob(handle, key, current, &size);
  if (err == ESP_OK && memcmp(current, value, size) == 0) {
    ESP_LOGI("storage", "save: %s: unchanged", key);
    goto storage_save_blob_exit;
  }

  err = nvs_set_blob(handle, key, value, size);
  if (err != ESP_OK) {
    ESP_LOGE("storage", "save: %s: %s", key, esp_err_to_name(err));
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE("storage", "commit: %s", esp_err_to_name(err));
  }

storage_save_blob_exit:
  free(current);
  nvs_close(handle);
}

void storage_init(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGE("storage", "init: %s", esp_err_to_name(err));
    return;
  }

  nvs_handle handle;
  err = nvs_open(SETTINGS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE("storage", "open: %s", esp_err_to_name(err));
    return;
  }

  storage_load_blob(handle, "wifi", &settings.wifi, sizeof(settings_wifi_t));
  storage_load_blob(handle, "brightness", &settings.brightness, sizeof(settings_brightness_t));
  storage_load_blob(handle, "color", &settings.color, sizeof(settings_color_t));

  nvs_close(handle);
}

void storage_wipe(void) {
  nvs_flash_erase();
}

void storage_save_wifi(void) {
  storage_save_blob("wifi", &settings.wifi, sizeof(settings_wifi_t));
}

void storage_save_brightness(void) {
  storage_save_blob("brightness", &settings.brightness, sizeof(settings_brightness_t));
}

void storage_save_color(void) {
  storage_save_blob("color", &settings.color, sizeof(settings_color_t));
}