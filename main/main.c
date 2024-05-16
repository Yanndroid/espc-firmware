#include "display.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "location.h"
#include "weather.h"
#include "wifi.h"
#include "ws2812b.h"
#include <sys/_stdint.h>
#include <sys/time.h>
#include <time.h>

void sync_rtc_with_ntp() {
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  const int retry_count = 150;


  while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
    /* ESP_LOGI("ntp", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    time(&now);
    localtime_r(&now, &timeinfo); */

    display_show_loading_next();

    time(&now);
    localtime_r(&now, &timeinfo);
  }

  /* if (retry < retry_count) {
    ESP_LOGI("ntp", "System time synchronized.");
    struct timeval tv = {.tv_sec = now};
    settimeofday(&tv, NULL);
  } else {
    ESP_LOGE("ntp", "Failed to synchronize system time with NTP server.");
  } */
}

uint8_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint8_t out_min, uint8_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t calc_brightness(bool night, time_t *now, weather_t *weather) {
  uint8_t br_min = 2;
  uint8_t br_max = 60;
  uint16_t margin = 7200;

  if (weather->sunrise - margin <= *now && *now <= weather->sunrise + margin) { // sunrise
    return map(*now - weather->sunrise + margin, 0, margin * 2, br_min, br_max);
  } else if (weather->sunset - margin <= *now && *now <= weather->sunset + margin) { // sunset
    return map(*now - weather->sunset + margin, 0, margin * 2, br_max, br_min);
  }

  if (night) { // night
    return br_min;
  } else { // day
    return br_max;
  }
}

void app_main() {
  display_init();

  initialise_wifi();

  sync_rtc_with_ntp();
  setenv("TZ", "UTC-2", 1);
  tzset();

  location_request();
  weather_request(location_get());

  weather_t *weather = weather_get();

  uint16_t hue = 5400;
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
      ws2812b_color_hsv(&color, hue += 2, 255, 255);
    }

    if (timeinfo.tm_sec % 30 < 2) {
      color_t sec_color;

      if (night) {
        sec_color = (color_t){.r = 255};
      } else {
        ws2812b_color_hsv(&sec_color, hue + (1 << 15), 240, 255);
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

    // ESP_LOGI("loop log", "free heap: %d, color hue: %d",
    // esp_get_free_heap_size(), hue);

    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}