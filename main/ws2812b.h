#pragma once

#include "esp_attr.h"
#include <sys/types.h>

typedef struct {
  uint8_t g;
  uint8_t r;
  uint8_t b;
} color_t;

void ws2812b_color_hsv(color_t *color, uint16_t hue, uint8_t sat, uint8_t val);

void ws2812b_init(void);

void ws2812b_clear(void);

void ws2812b_set_brightness(float brightness);

void ws2812b_set_pixel(uint8_t index, color_t color);

void IRAM_ATTR ws2812b_show(void);