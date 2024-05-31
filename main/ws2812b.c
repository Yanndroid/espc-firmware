#include "ws2812b.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portable.h"
#include "portmacro.h"
#include "rom/ets_sys.h"
#include <string.h>

#define WS2812B_PIN 5
#define WS2812B_NUM 59

static color_t ws2812b_pixels[WS2812B_NUM];
static uint8_t ws2812b_brightness = 255;

void ws2812b_color_hsv(color_t *color, uint16_t hue, uint8_t sat) {
  uint8_t r, g, b;
  hue = (hue * 1530L + 32768) / 65536;

  if (hue < 510) {
    b = 0;
    if (hue < 255) {
      r = 255;
      g = hue;
    } else {
      r = 510 - hue;
      g = 255;
    }
  } else if (hue < 1020) {
    r = 0;
    if (hue < 765) {
      g = 255;
      b = hue - 510;
    } else {
      g = 1020 - hue;
      b = 255;
    }
  } else if (hue < 1530) {
    g = 0;
    if (hue < 1275) {
      r = hue - 1020;
      b = 255;
    } else {
      r = 255;
      b = 1530 - hue;
    }
  } else {
    r = 255;
    g = b = 0;
  }

  uint16_t s1 = 1 + sat;
  uint8_t s2 = 255 - sat;
  color->r = (((r * s1) >> 8) + s2);
  color->g = (((g * s1) >> 8) + s2);
  color->b = (((b * s1) >> 8) + s2);
}

void ws2812b_init(void) {
  gpio_config_t led_gpio = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = 1ULL << WS2812B_PIN,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
  };
  gpio_config(&led_gpio);

  ws2812b_clear();
}

void ws2812b_clear(void) { memset(&ws2812b_pixels, 0, sizeof(ws2812b_pixels)); }

void ws2812b_set_brightness(uint8_t brightness) { ws2812b_brightness = brightness; }

void ws2812b_set_pixel(uint8_t index, color_t color) {
  if (index < WS2812B_NUM)
    ws2812b_pixels[index] = color;
}

void IRAM_ATTR ws2812b_show(void) {
  uint8_t bytes[sizeof(ws2812b_pixels)];
  for (int i = 0; i < sizeof(ws2812b_pixels); i++) {
    bytes[i] = (*((uint8_t *)ws2812b_pixels + i) * ws2812b_brightness) >> 8;
  }

  vPortEnterCritical();
  vPortEndScheduler(); // TODO: check if only needed once
  vPortETSIntrLock();

  /* for (int i = 0; i < sizeof(ws2812b_pixels); i++) {
    uint8_t mask = 0x80;
    uint8_t byte = (*((uint8_t *)ws2812b_pixels + i) * ws2812b_brightness) >> 8; */

  for (int i = 0; i < sizeof(bytes); i++) {
    uint8_t mask = 0x80;
    uint8_t byte = bytes[i];

    for (int i = 0; i < 8; i++) {
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1ul << WS2812B_PIN);
      if (byte & mask) {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
      } else {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
      }
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1ul << WS2812B_PIN);
      if (byte & mask) {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
      } else {
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;");
      }
      mask >>= 1;
    }
  }

  vPortETSIntrUnlock();
  vPortExitCritical();

  vTaskDelay(10 / portTICK_PERIOD_MS); // TODO: remove maybe
}