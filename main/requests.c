#include "requests.h"
#include <esp_http_client.h>

requests_json_parser requests_parser = NULL;

esp_err_t requests_http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    return ESP_FAIL;
  case HTTP_EVENT_ON_DATA: {

    cJSON *root = cJSON_Parse(evt->data);
    if (root == NULL) {
      return ESP_FAIL;
    }

    requests_parser(root);

    cJSON_Delete(root);
    break;
  }
  default:
    break;
  }
  return ESP_OK;
}

esp_err_t requests_get_json(const char *url, requests_json_parser parser) {
  if (parser == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  requests_parser = parser;

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = requests_http_event_handler,
      .cert_pem = NULL,
      .skip_cert_common_name_check = true,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_err_t err = esp_http_client_perform(client);
  if (esp_http_client_get_status_code(client) != 200) {
    err = ESP_FAIL;
  }

  esp_http_client_cleanup(client);
  requests_parser = NULL;
  return err;
}