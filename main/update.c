#include "update.h"

#include "sdkconfig.h"
#include "sodium/crypto_sign.h"
#include <esp_ota_ops.h>
#include <string.h>

static const char *PUBLIC_KEY = CONFIG_OTA_PUBLIC_KEY;

#define OTA_BUF_SIZE CONFIG_OTA_BUF_SIZE

static void hex2bin(const char *hex, unsigned char *bin, uint8_t bin_len) {
  uint8_t hex_len = strlen(hex);
  if (hex_len % 2 != 0 || hex_len / 2 > bin_len) {
    return;
  }
  for (uint8_t i = 0; i < hex_len / 2; i++) {
    sscanf(hex + 2 * i, "%2hhx", &bin[i]);
  }
}

static void http_cleanup(esp_http_client_handle_t client) {
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}

esp_err_t update_ota(update_data_t *data) {
  esp_http_client_config_t config = {
      .url = data->url,
      .event_handler = data->event_handle,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) {
    return ESP_FAIL;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    return err;
  }
  esp_http_client_fetch_headers(client);

  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = NULL;
  update_partition = esp_ota_get_next_update_partition(NULL);
  if (update_partition == NULL) {
    http_cleanup(client);
    return ESP_FAIL;
  }

  err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
  if (err != ESP_OK) {
    http_cleanup(client);
    return err;
  }

  esp_err_t ota_write_err = ESP_OK;
  char *upgrade_data_buf = (char *)malloc(OTA_BUF_SIZE);
  if (!upgrade_data_buf) {
    return ESP_ERR_NO_MEM;
  }

  crypto_sign_state state;
  crypto_sign_init(&state);

  while (1) {
    int data_read = esp_http_client_read(client, upgrade_data_buf, OTA_BUF_SIZE);
    if (data_read == 0) {
      break;
    }
    if (data_read < 0) {
      break;
    }
    if (data_read > 0) {
      ota_write_err = esp_ota_write(update_handle, (const void *)upgrade_data_buf, data_read);
      crypto_sign_update(&state, (const unsigned char *)upgrade_data_buf, data_read);
      if (ota_write_err != ESP_OK) {
        break;
      }
    }
  }
  free(upgrade_data_buf);
  http_cleanup(client);

  unsigned char public_key_bin[crypto_sign_PUBLICKEYBYTES];
  hex2bin(PUBLIC_KEY, public_key_bin, crypto_sign_PUBLICKEYBYTES);
  // sodium_hex2bin(public_key_bin, sizeof(public_key_bin), PUBLIC_KEY, strlen(PUBLIC_KEY), NULL, NULL, NULL);

  unsigned char signature_bin[crypto_sign_BYTES];
  hex2bin(data->signature, signature_bin, crypto_sign_BYTES);
  // sodium_hex2bin(signature_bin, sizeof(signature_bin), data->signature, strlen(data->signature), NULL, NULL, NULL);

  int signature_err = crypto_sign_final_verify(&state, signature_bin, public_key_bin);

  esp_err_t ota_end_err = esp_ota_end(update_handle);

  if (ota_write_err != ESP_OK) {
    return ota_write_err;
  } else if (ota_end_err != ESP_OK) {
    return ota_end_err;
  } else if (signature_err != 0) {
    return ESP_ERR_INVALID_STATE;
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}
