#include "display.h"
#include "location.h"
#include "settings.h"
#include "weather.h"
#include "wifi.h"
#include "ws2812b.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/apps/sntp.h"
#include <string.h>
#include <sys/_stdint.h>
#include <sys/time.h>

void sync_rtc_with_ntp() {
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  time_t now = 0;
  struct tm timeinfo = {0};

  while (timeinfo.tm_year <= 70) {
    display_show_loading_next();

    time(&now);
    localtime_r(&now, &timeinfo);
  }
}

uint8_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint8_t out_min, uint8_t out_max) { return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; }

uint8_t calc_brightness(bool night, time_t *now, weather_t *weather) {
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

void start_wifi(char ssid[32], char username[32], char password[64]) {
  if (wifi_station(ssid, username, password)) {
    strcpy(settings.wifi.ssid, ssid);
    strcpy(settings.wifi.username, username);
    strcpy(settings.wifi.password, password);
    // TODO: save wifi struct to nvs
    return;
  }

  settings.wifi.ssid[0] = '\0';
  settings.wifi.username[0] = '\0';
  settings.wifi.password[0] = '\0';

  wifi_ap(settings.device_name, "yanndroid");

  display_show_app(); // TODO: display "APP"
}

void app_main() {
  // TODO: load nvs

  display_init();
  display_set_brightness(settings.brightness.max, false);
  // display_show_boot();

  esp_netif_init();
  esp_event_loop_create_default();

  wifi_init();
  start_wifi(settings.wifi.ssid, settings.wifi.username, settings.wifi.password);
  while (strlen(settings.wifi.ssid) == 0) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  // TDOD: show ip address

  sync_rtc_with_ntp();
  setenv("TZ", "UTC-2", 1);
  tzset();

  location_request();
  weather_request(location_get());

  weather_t *weather = weather_get();

  uint16_t hue = settings.color.hue;
  color_t color;

  time_t now;
  struct tm timeinfo;

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
      weather_request(location_get());
    }

    //ESP_LOGI("main", "free heap: %d", esp_get_free_heap_size());

    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}