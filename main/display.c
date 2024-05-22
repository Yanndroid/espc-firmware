#include "display.h"
#include "icons.h"
#include "ws2812b.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

// 0,1,2,3,4,5,6,7,8,9,A,b,C,d,E,F, ,°,-,_,P
static const uint8_t display_digit_masks[] = {0x77, 0x11, 0x6b, 0x3b, 0x1d, 0x3e, 0x7e, 0x13, 0x7f, 0x3f, 0x5f, 0x7c, 0x66, 0x79, 0x6e, 0x4e, 0x00, 0x0f, 0x02, 0x08, 0x4f};

const static color_t color_off = {0, 0, 0};

typedef enum {
  SEGMENT_1 = 0,
  SEGMENT_2 = 14,
  SEGMENT_3 = 31,
  SEGMENT_4 = 45,
} segment_t;

static void display_show_temperature(weather_t *weather, color_t color);
static int display_blank_0(uint8_t digit);
static void display_segment_digit(segment_t segment, uint8_t digit, color_t color);
static void display_segment_mask(segment_t segment, uint8_t mask, color_t color);
static void display_segments_icon(segment_t start_segment, uint32_t icon);
static void display_dots(bool time_dots, bool date_dot, color_t color);
static void display_loading(uint8_t frame, color_t color_one, color_t color_two);

void display_init(void) {
  ws2812b_init();
  ws2812b_show();
}

void display_set_brightness(uint8_t brightness, bool update) {
  ws2812b_set_brightness(brightness);
  if (update)
    ws2812b_show();
}

void display_show_boot(void) {
  display_segment_digit(SEGMENT_1, 8, (color_t){.r = 255, .g = 255, .b = 255});
  display_segment_digit(SEGMENT_2, 8, (color_t){.b = 255});
  display_segment_digit(SEGMENT_3, 8, (color_t){.g = 255});
  display_segment_digit(SEGMENT_4, 8, (color_t){.r = 255});
  display_dots(true, true, (color_t){.r = 255, .g = 255, .b = 255});
  ws2812b_show();
}

void display_show_app(void) {
  display_segment_digit(SEGMENT_1, 16, color_off);
  display_segment_digit(SEGMENT_2, 20, (color_t){.b = 255});
  display_segment_digit(SEGMENT_3, 20, (color_t){.g = 255});
  display_segment_digit(SEGMENT_4, 10, (color_t){.r = 255});
  display_dots(false, false, color_off);
  ws2812b_show();
}

void display_show_test(uint8_t a, uint8_t b, uint8_t c, uint8_t d, color_t color) {
  display_segment_digit(SEGMENT_1, d, color);
  display_segment_digit(SEGMENT_2, c, color);
  display_segment_digit(SEGMENT_3, b, color);
  display_segment_digit(SEGMENT_4, a, color);
  display_dots(true, true, color);
  ws2812b_show();
}

void display_show_time(struct tm *timeinfo, color_t color) {
  display_segment_digit(SEGMENT_1, timeinfo->tm_min % 10, color);
  display_segment_digit(SEGMENT_2, timeinfo->tm_min / 10, color);
  display_segment_digit(SEGMENT_3, timeinfo->tm_hour % 10, color);
  display_segment_digit(SEGMENT_4, display_blank_0(timeinfo->tm_hour / 10), color);
  display_dots(timeinfo->tm_sec % 2 == 0, false, color);
  ws2812b_show();
}

void display_show_date(struct tm *timeinfo, color_t day_color, color_t mon_color) {
  display_segment_digit(SEGMENT_1, (timeinfo->tm_mon + 1) % 10, mon_color);
  display_segment_digit(SEGMENT_2, display_blank_0((timeinfo->tm_mon + 1) / 10), mon_color);
  display_segment_digit(SEGMENT_3, timeinfo->tm_mday % 10, day_color);
  display_segment_digit(SEGMENT_4, display_blank_0(timeinfo->tm_mday / 10), day_color);
  display_dots(false, true, day_color);
  ws2812b_show();
}

void display_show_temperature_degree(weather_t *weather, color_t color) {
  display_segment_digit(SEGMENT_1, 12, color);
  display_segment_digit(SEGMENT_2, 17, color);
  display_show_temperature(weather, color);
}

void display_show_temperature_weather(weather_t *weather, color_t color) {
  display_segments_icon(SEGMENT_1, icons_get_weather_icon(weather->weather_code));

  display_show_temperature(weather, color);
}

void display_show_loading_next() {
  static uint8_t frame = 0;
  display_loading(frame, (color_t){.r = 0, .g = 255, .b = 30}, (color_t){.r = 0, .g = 30, .b = 255});
  frame = (frame + 1) % 24;
  vTaskDelay(1000 / 12 / portTICK_PERIOD_MS);
}

static void display_show_temperature(weather_t *weather, color_t color) {
  int8_t temp = weather->temperature;
  if (temp < 0) { // XOR minus and tens digit
    uint8_t abs_temp = abs(temp);
    display_segment_digit(SEGMENT_3, abs_temp % 10, color);
    uint8_t mask = display_digit_masks[19] ^ display_digit_masks[abs_temp / 10];
    display_segment_mask(SEGMENT_4, mask, color);
  } else {
    display_segment_digit(SEGMENT_3, temp % 10, color);
    display_segment_digit(SEGMENT_4, display_blank_0(temp / 10), color);
  }
  display_dots(false, false, color_off);
  ws2812b_show();
}

static int display_blank_0(uint8_t digit) { return (digit != 0) ? digit : 16; }

static void display_segment_digit(segment_t segment, uint8_t digit, color_t color) {
  if (digit < sizeof(display_digit_masks)) {
    display_segment_mask(segment, display_digit_masks[digit], color);
  }
}

static void display_segment_mask(segment_t segment, uint8_t mask, color_t color) {
  for (uint8_t i = 0; i < 7; i++) {
    if (0 == (mask & ((uint8_t)1 << i))) {
      ws2812b_set_pixel(segment + 2 * i, color_off);
      ws2812b_set_pixel(segment + 2 * i + 1, color_off);
    } else {
      ws2812b_set_pixel(segment + 2 * i, color);
      ws2812b_set_pixel(segment + 2 * i + 1, color);
    }
  }
}

static void display_segments_icon(segment_t start_segment, uint32_t icon) {
  color_t colors[] = {{}, {255, 255, 255}, {.r = 255, .g = 191}, {.g = 179, .b = 255}};
  for (uint8_t i = 0; i < 14; i++) {
    uint8_t color_index = (icon >> (2 * i)) & 0x03;
    color_t color = colors[color_index];
    ws2812b_set_pixel(start_segment + 2 * i, color);
    ws2812b_set_pixel(start_segment + 2 * i + 1, color);
  }
}

static void display_dots(bool time_dots, bool date_dot, color_t color) {
  ws2812b_set_pixel(28, date_dot ? color : color_off);
  ws2812b_set_pixel(29, time_dots ? color : color_off);
  ws2812b_set_pixel(30, time_dots ? color : color_off);
}

static void display_loading(uint8_t frame, color_t color_one, color_t color_two) {
  uint8_t anim[] = {1, 0, 8, 9, 10, 11, 24, 25, 41, 42, 55, 56, 57, 58, 50, 49, 48, 47, 34, 33, 17, 16, 3, 2};
  uint8_t size = sizeof(anim);
  if (frame >= size)
    return;

  ws2812b_clear();
  ws2812b_set_pixel(anim[frame], color_one);
  ws2812b_set_pixel(anim[(frame + 1) % size], color_one);

  ws2812b_set_pixel(anim[(frame + size / 2) % size], color_two);
  ws2812b_set_pixel(anim[(frame + 1 + size / 2) % size], color_two);
  ws2812b_show();
}