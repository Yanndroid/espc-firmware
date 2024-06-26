#include "wifi.h"
#include "display.h"
#include "settings.h"

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/event_groups.h"
#include <string.h>

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (wifi_event_group) {
      xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }

    system_event_sta_disconnected_t *event = (system_event_sta_disconnected_t *)event_data;
    switch (event->reason) {
    case WIFI_REASON_NOT_AUTHED:
      return;
    case WIFI_REASON_BASIC_RATE_NOT_SUPPORT:
      esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
      break;
    }

    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    if (wifi_event_group) {
      xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }
}

void wifi_init(void) {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  mac[0] = 0x44;
  mac[1] = 0x44;
  esp_base_mac_addr_set(mac);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);

  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
}

void wifi_ap(char ssid[32], char password[64]) {
  esp_wifi_stop();
  wifi_config_t wifi_config = {
      .ap = {.ssid_len = strlen(ssid), .max_connection = 1, .authmode = (strlen(password) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK},
  };
  strcpy((char *)wifi_config.ap.ssid, ssid);
  strcpy((char *)wifi_config.ap.password, password);

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
  esp_wifi_start();
  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, settings.device_name);
}

bool wifi_station(char ssid[32], char username[32], char password[64]) {
  if (strlen(ssid) == 0) {
    return false;
  }
  esp_wifi_stop();

  wifi_event_group = xEventGroupCreate();

  wifi_config_t wifi_config = {.sta = {}};
  strcpy((char *)wifi_config.sta.ssid, ssid);
  strcpy((char *)wifi_config.sta.password, password);

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

  if (strlen(username) != 0) {
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
    esp_wifi_sta_wpa2_ent_enable();
  }

  esp_wifi_start();
  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, settings.device_name);

  TickType_t start_time = xTaskGetTickCount();
  while (!(xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT)) {
    display_show_loading_next();

    if ((xTaskGetTickCount() - start_time) >= pdMS_TO_TICKS(10000)) {
      esp_wifi_disconnect();
      esp_wifi_stop();
      vEventGroupDelete(wifi_event_group);
      wifi_event_group = NULL;
      return false;
    }
  }

  vEventGroupDelete(wifi_event_group);
  wifi_event_group = NULL;
  return true;
}