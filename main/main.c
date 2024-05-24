#include "cJSON.h"
#include "coap.h"
#include "display.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "location.h"
#include "lwip/apps/sntp.h"
#include "settings.h"
#include "weather.h"
#include "wifi.h"
#include "ws2812b.h"
#include <string.h>
#include <sys/_stdint.h>
#include <sys/time.h>

static uint8_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint8_t out_min, uint8_t out_max) {
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
    // TODO: update display
  }

  cJSON *color = cJSON_GetObjectItem(request, "color");
  if (color != NULL) {
    settings.color.hue = cJSON_GetObjectItem(color, "hue")->valueint;
    settings.color.sat = cJSON_GetObjectItem(color, "sat")->valueint;
  }
}

void app_main() {
  // settings
  // TODO: load nvs

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
      // TODO: save wifi struct to nvs
      break;
    }

    settings.wifi.ssid[0] = '\0';
    settings.wifi.username[0] = '\0';
    settings.wifi.password[0] = '\0';

    wifi_ap(settings.device_name, "yanndroid");
    display_show_app();

    while (strlen(settings.wifi.ssid) == 0) {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }

  // sync time
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  // show ip address
  tcpip_adapter_ip_info_t ip_info;
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
  display_show_ip((ip_info.ip.addr >> 16) & 0xFF, (ip_info.ip.addr >> 24) & 0xFF);

  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // location and weather
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
    time(&now);
    localtime_r(&now, &timeinfo);

    bool night = weather->sunset < now || now < weather->sunrise;

    display_set_brightness(calc_brightness(night, &now, weather), false);

    if (night) {
      color = (color_t){.r = 255};
    } else {
      ws2812b_color_hsv(&color, hue += 2, settings.color.sat);
    }

    if (timeinfo.tm_sec % 30 < 2) {
      color_t sec_color;

      if (night) {
        sec_color = color;
      } else {
        ws2812b_color_hsv(&sec_color, hue + (1 << 15), settings.color.sat);
      }

      display_show_date(&timeinfo, color, sec_color);
    } else if (timeinfo.tm_sec % 30 < 4) {

      if (night) {
        display_show_temperature_degree(weather, color);
      } else {
        display_show_temperature_weather(weather, color);
      }

    } else {
      display_show_time(&timeinfo, color);
    }

    if (now > weather->next_update) {
      if (weather_request(location_get()) == ESP_FAIL) {
        weather->next_update += 300;
      }
    }

    // ESP_LOGI("main", "free heap: %d", esp_get_free_heap_size());

    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}