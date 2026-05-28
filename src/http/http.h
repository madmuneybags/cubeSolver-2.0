#include "esp_log.h"
#include "esp_http_client.h"

#define MAX_HTTP_RESPONSE 512

static esp_err_t _http_event_handler(esp_http_client_event_t *event);
bool send_post_request(const char *payload, char *response_buf, int response_buf_len);