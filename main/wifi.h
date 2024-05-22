#pragma once

#include <stdbool.h>

void wifi_init(void);

void wifi_ap(char ssid[32], char password[64]);

bool wifi_station(char ssid[32], char username[32], char password[64]);