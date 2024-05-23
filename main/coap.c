#include "coap.h"

#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <coap.h>
#include <sys/socket.h>

extern void coap_get_handler(cJSON *request, cJSON *response);
extern void coap_put_handler(cJSON *request, cJSON *response);

static void coap_handle_get(coap_context_t *ctx, coap_resource_t *resource, coap_session_t *session, coap_pdu_t *request, coap_binary_t *token, coap_string_t *query, coap_pdu_t *response) {
  size_t size;
  unsigned char *data;
  coap_get_data(request, &size, &data);

  cJSON *response_json = cJSON_CreateObject();
  cJSON *request_json = cJSON_Parse((const char *)data);
  if (response_json == NULL || request_json == NULL) {
    response->code = COAP_RESPONSE_400;
    return;
  }

  coap_get_handler(request_json, response_json);
  char *response_data = cJSON_PrintUnformatted(response_json);

  unsigned char buf[3];
  coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
  coap_add_data(response, strlen(response_data), (unsigned char *)response_data);
  response->code = COAP_RESPONSE_200;

  cJSON_Delete(response_json);
  cJSON_Delete(request_json);
}

static void coap_handle_put(coap_context_t *ctx, coap_resource_t *resource, coap_session_t *session, coap_pdu_t *request, coap_binary_t *token, coap_string_t *query, coap_pdu_t *response) {
  size_t size;
  unsigned char *data;
  coap_get_data(request, &size, &data);

  cJSON *request_json = cJSON_Parse((const char *)data);
  if (request_json == NULL) {
    response->code = COAP_RESPONSE_400;
    return;
  }

  coap_put_handler(request_json, NULL);

  response->code = COAP_RESPONSE_200;

  cJSON_Delete(request_json);
}

static void coap_thread(void *pvParameters) {
  coap_address_t address;
  coap_address_init(&address);
  address.addr.sin.sin_family = AF_INET;
  address.addr.sin.sin_addr.s_addr = INADDR_ANY;
  address.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);

  coap_context_t *context = coap_new_context(&address);
  if (!context) {
    vTaskDelete(NULL);
    return;
  }

  coap_resource_t *resource = coap_resource_init(NULL, 0);
  if (!resource)
    goto coap_thread_exit;
  coap_register_handler(resource, COAP_REQUEST_GET, coap_handle_get);
  coap_register_handler(resource, COAP_REQUEST_PUT, coap_handle_put);
  coap_add_resource(context, resource);

  while (1) {
    coap_run_once(context, 0);
  }

coap_thread_exit:
  coap_free_context(context);
  coap_cleanup();
  vTaskDelete(NULL);
}

void coap_init(void) {
  xTaskCreate(coap_thread, "coap", 1024 * 5, NULL, 5, NULL);
}
