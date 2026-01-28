#ifndef MRBC_ESP32_HTTP_CLIENT_H
#define MRBC_ESP32_HTTP_CLIENT_H

#include "mrubyc.h"
#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 2048

extern char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1];

esp_err_t http_event_handler(esp_http_client_event_t *evt);
void mrbc_esp32_httpclient_gem_init(struct VM*);
  
#endif // MRBC_ESP32_HTTP_CLIENT_H
