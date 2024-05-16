#include "icons.h"

typedef struct {
  uint8_t weather_code;
  uint8_t icon_index;
} icons_weather_map_t;

const static icons_weather_map_t icons_weather_map[] = {{0, 0},   {1, 1},   {2, 1},   {3, 2},   {45, 3},  {48, 4},  {51, 5},
                                                        {53, 5},  {61, 5},  {63, 5},  {55, 6},  {65, 6},  {56, 7},  {57, 7},
                                                        {66, 7},  {67, 7},  {71, 8},  {73, 8},  {75, 9},  {77, 10}, {80, 11},
                                                        {81, 11}, {82, 12}, {85, 13}, {86, 14}, {95, 15}, {96, 15}, {99, 16}};

const static uint32_t icons_weather_icons[] = {0xaa,      0x510056a, 0x150045,  0x1100440, 0x110046a, 0xc153045,
                                               0xcd53345, 0xdd53745, 0x4151045, 0x4551145, 0x4d51345, 0xc15304a,
                                               0xcd5334a, 0x415104a, 0x455114a, 0x952045,  0xc952345};

uint32_t icons_get_weather_icon(uint8_t weather_code) {
  for (uint8_t i = 0; i < sizeof(icons_weather_map) / sizeof(icons_weather_map_t); i++) {
    if (icons_weather_map[i].weather_code == weather_code) {
      return icons_weather_icons[icons_weather_map[i].icon_index];
    }
  }
  return 0x0;
}