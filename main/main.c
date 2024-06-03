#include "cJSON.h"
#include "coap.h"
#include "display.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "location.h"
#include "lwip/apps/sntp.h"
#include "pending_task.h"
#include "sdkconfig.h"
#include "settings.h"
#include "update.h"
#include "weather.h"
#include "wifi.h"
#include "ws2812b.h"
#include <string.h>
#include <sys/_stdint.h>
#include <sys/time.h>

static void main_online_mode(void);
static int map(int x, int in_min, int in_max, int out_min, int out_max);
static uint8_t calc_brightness(bool night, time_t *now, weather_t *weather);
static void pending_task_locate(const void *args);
static void pending_task_update(const void *args);
static void pending_task_restart(const void *args);
static void pending_task_wipe_nvs(const void *args);
static esp_err_t update_event_handler(esp_http_client_event_t *evt);

void app_main() {
  // settings
  settings_init();

  // display
  display_init();
  display_set_brightness(settings.brightness.max, false);

  // network
  esp_netif_init();
  esp_event_loop_create_default();

  // coap
  coap_init();

  // wifi
  wifi_init();
  while (1) {
    if (wifi_station(settings.wifi.ssid, settings.wifi.username, settings.wifi.password)) {
      settings_save_wifi();
      break;
    }

    settings.wifi.ssid[0] = '\0';
    settings.wifi.username[0] = '\0';
    settings.wifi.password[0] = '\0';

    wifi_ap(settings.device_name, CONFIG_AP_PASSWORD);
    display_show_app();

    while (strlen(settings.wifi.ssid) == 0) {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }

  main_online_mode();
}

static void main_online_mode(void) {
  // sync time
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  // show ip address
  tcpip_adapter_ip_info_t ip_info;
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
  display_show_ip((ip_info.ip.addr >> 16) & 0xFF, (ip_info.ip.addr >> 24) & 0xFF);

  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // get location and weather
  while (location_request() == ESP_FAIL) {
    display_show_loading_next();
  }
  while (weather_request(location_get()) == ESP_FAIL) {
    display_show_loading_next();
  }

  weather_t *weather = weather_get();

  // wait for time sync
  time_t now = 0;
  struct tm timeinfo = {.tm_year = 0};

  while (timeinfo.tm_year <= 70) {
    display_show_loading_next();

    time(&now);
    localtime_r(&now, &timeinfo);
  }

  // set timezone
  setenv("TZ", "UTC-2", 1);
  tzset();

  // main loop
  uint16_t hue = settings.color.hue;
  color_t color;

  while (1) {
    // process pending task
    pending_task_run();

    // get time
    time(&now);
    localtime_r(&now, &timeinfo);

    // brightness
    bool night = weather->sunset < now || now < weather->sunrise;
    display_set_brightness(calc_brightness(night, &now, weather), false);

    // color
    if (night) {
      color = (color_t){.r = 255};
    } else {
      ws2812b_color_hsv(&color, hue += 2, settings.color.sat);
    }

    // display
    if (timeinfo.tm_sec % 30 < 2) {
      // show date
      color_t sec_color;
      if (night) {
        sec_color = color;
      } else {
        ws2812b_color_hsv(&sec_color, hue + (1 << 15), settings.color.sat);
      }
      display_show_date(&timeinfo, color, sec_color);
    } else if (timeinfo.tm_sec % 30 < 4) {
      // show weather
      if (night) {
        display_show_temperature_degree(weather, color);
      } else {
        display_show_temperature_weather(weather, color);
      }
    } else {
      // show time
      display_show_time(&timeinfo, color);
    }

    // update weather
    if (now > weather->next_update) {
      if (weather_request(location_get()) == ESP_FAIL) {
        weather->next_update += 300;
      }
    }

    // ESP_LOGI("main", "free heap: %d", esp_get_free_heap_size());
    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

static int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static uint8_t calc_brightness(bool night, time_t *now, weather_t *weather) {
  if (weather->sunrise - settings.brightness.margin <= *now && *now <= weather->sunrise + settings.brightness.margin) { // sunrise
    return map(*now - weather->sunrise + settings.brightness.margin, 0, settings.brightness.margin * 2, settings.brightness.min, settings.brightness.max);
  } else if (weather->sunset - settings.brightness.margin <= *now && *now <= weather->sunset + settings.brightness.margin) { // sunset
    return map(*now - weather->sunset + settings.brightness.margin, 0, settings.brightness.margin * 2, settings.brightness.max, settings.brightness.min);
  }

  if (night) { // night
    return settings.brightness.min;
  } else { // day
    return settings.brightness.max;
  }
}

static void pending_task_locate(const void *args) {
  display_set_brightness(settings.brightness.max, false);
  for (int i = 0; i < 24; i++) {
    display_show_loading_next();
  }
}

static void pending_task_update(const void *args) {
  display_set_brightness(settings.brightness.max, false);
  display_circle_progress(23, (color_t){.g = 255, .b = 100});

  esp_err_t err = update_ota((update_data_t *)args);
  free(((update_data_t *)args)->url);
  free(((update_data_t *)args)->signature);

  if (err == ESP_OK) {
    display_circle_progress(23, (color_t){.g = 255});
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
  } else {
    display_circle_progress(23, (color_t){.r = 255});
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

static void pending_task_restart(const void *args) {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  esp_restart();
}

static void pending_task_wipe_nvs(const void *args) {
  settings_wipe();
  pending_task_restart(args);
}

static esp_err_t update_event_handler(esp_http_client_event_t *evt) {
  static int content_length = 0;
  static int content_received = 0;

  switch (evt->event_id) {
  case HTTP_EVENT_ON_HEADER:
    if (strcmp(evt->header_key, "Content-Length") == 0) {
      content_length = atoi(evt->header_value);
    }
    break;
  case HTTP_EVENT_ON_DATA:
    content_received += evt->data_len;
    if (content_length > 0) {
      uint8_t progress = map(content_received, 0, content_length, 0, 23);
      display_circle_progress(progress, (color_t){.r = 255, .g = 100});
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    content_length = 0;
    content_received = 0;
    break;
  default:
    break;
  }
  return ESP_OK;
}

void coap_get_handler(cJSON *request, cJSON *response) {
  for (int i = 0; i < cJSON_GetArraySize(request); i++) {
    cJSON *item = cJSON_GetArrayItem(request, i);
    if (!item) {
      continue;
    }

    if (strcmp(item->valuestring, "device") == 0) {
      cJSON *device = cJSON_CreateObject();
      cJSON_AddItemToObject(response, "device", device);
      cJSON_AddStringToObject(device, "name", settings.device_name);
      cJSON_AddNumberToObject(device, "heap", esp_get_free_heap_size());

      uint8_t mac[6];
      esp_read_mac(mac, ESP_MAC_WIFI_STA);
      char mac_str[18];
      snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      cJSON_AddStringToObject(device, "mac", mac_str);

    } else if (strcmp(item->valuestring, "wifi") == 0) {
      cJSON *wifi = cJSON_CreateObject();
      cJSON_AddItemToObject(response, "wifi", wifi);
      cJSON_AddStringToObject(wifi, "ssid", settings.wifi.ssid);
    } else if (strcmp(item->valuestring, "brightness") == 0) {
      cJSON *brightness = cJSON_CreateObject();
      cJSON_AddItemToObject(response, "brightness", brightness);
      cJSON_AddNumberToObject(brightness, "max", settings.brightness.max);
      cJSON_AddNumberToObject(brightness, "min", settings.brightness.min);
      cJSON_AddNumberToObject(brightness, "margin", settings.brightness.margin);
    } else if (strcmp(item->valuestring, "color") == 0) {
      cJSON *color = cJSON_CreateObject();
      cJSON_AddItemToObject(response, "color", color);
      cJSON_AddNumberToObject(color, "hue", settings.color.hue);
      cJSON_AddNumberToObject(color, "sat", settings.color.sat);
    }
  }
}

void coap_put_handler(cJSON *request, cJSON *response) {
  if (strlen(settings.wifi.ssid) == 0) {
    cJSON *wifi = cJSON_GetObjectItem(request, "wifi");
    if (wifi != NULL) {
      strcpy(settings.wifi.ssid, cJSON_GetObjectItem(wifi, "ssid")->valuestring);
      strcpy(settings.wifi.username, cJSON_GetObjectItem(wifi, "username")->valuestring);
      strcpy(settings.wifi.password, cJSON_GetObjectItem(wifi, "password")->valuestring);
    }
  }

  cJSON *brightness = cJSON_GetObjectItem(request, "brightness");
  if (brightness != NULL) {
    settings.brightness.max = cJSON_GetObjectItem(brightness, "max")->valueint;
    settings.brightness.min = cJSON_GetObjectItem(brightness, "min")->valueint;
    settings.brightness.margin = cJSON_GetObjectItem(brightness, "margin")->valueint;
    settings_save_brightness();
  }

  cJSON *color = cJSON_GetObjectItem(request, "color");
  if (color != NULL) {
    settings.color.hue = cJSON_GetObjectItem(color, "hue")->valueint;
    settings.color.sat = cJSON_GetObjectItem(color, "sat")->valueint;
    settings_save_color();
  }

  cJSON *update = cJSON_GetObjectItem(request, "update");
  if (update != NULL && !pending_task_get_method()) {
    update_data_t *update_data = malloc(sizeof(update_data_t));
    update_data->url = strdup(cJSON_GetObjectItem(update, "url")->valuestring);
    update_data->signature = strdup(cJSON_GetObjectItem(update, "signature")->valuestring);
    update_data->event_handle = update_event_handler;
    pending_task_set(pending_task_update, update_data);
  }

  cJSON *reset = cJSON_GetObjectItem(request, "reset");
  if (reset != NULL && !pending_task_get_method()) {
    pending_task_set(pending_task_wipe_nvs, NULL);
  }

  cJSON *restart = cJSON_GetObjectItem(request, "restart");
  if (restart != NULL && !pending_task_get_method()) {
    pending_task_set(pending_task_restart, NULL);
  }

  cJSON *locate = cJSON_GetObjectItem(request, "locate");
  if (locate != NULL && !pending_task_get_method()) {
    pending_task_set(pending_task_locate, NULL);
  }
}