#include "settings.h"

#include "sdkconfig.h"

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