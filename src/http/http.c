//#include <string.h>
#include "http.h"



typedef struct {
    char *buffer;
    int buffer_len;
    int bytes_written;
} http_response_t;


// the message response is not sent all at once, it is sent back in chunks.
// the data needs to be appended together
static esp_err_t _http_event_handler(esp_http_client_event_t *event)
{
    http_response_t *response = (http_response_t *)event->user_data;
    // each response event has an id
    switch (event->event_id){
        case HTTP_EVENT_ON_DATA:
            if (response && event->data && event->data_len > 0){
                int remaining = response->buffer_len - response->bytes_written - 1;

                if (remaining > 0){
                    int copy_len = event->data_len;
                    if ( copy_len > remaining) copy_len = remaining;

                    memcpy(response->buffer + response->bytes_written, event->data, copy_len);

                    response->bytes_written += copy_len;
                    response->buffer[response->bytes_written] = '\0';
                }
            }
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGE("HTTP", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("HTTP", "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGI("HTTP", "EVENT_NOT_RECOGNIZED");
        
    }
    return ESP_OK;
}

bool send_post_request(const char *payload, char *response_buf, int response_buf_len)
{
    if (payload == NULL || response_buf == NULL || response_buf_len <= 0){
        return false;
    }

    response_buf[0] = '\0';

    http_response_t response = {
        .buffer = response_buf,
        .buffer_len = response_buf_len,
        .bytes_written = 0
    };

    esp_http_client_config_t config = {
        .url = "https://execute-api.amazonaws.com/default",
        .event_handler = _http_event_handler,
        .user_data = &response,
        .timeout_ms = 180000
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE("HTTP", "Failed to init HTTP client");
        return false;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    char post_data[128];

    snprintf(post_data, sizeof(post_data), "{\"key1\":\"%s\"}", payload);

    // sends the post request
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t result = esp_http_client_perform(client);

    int response_code = esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);

    if(result = ESP_OK && response_code == 200){
        return true;
    }

    ESP_LOGE("TAG", "HTTP POST failed, err=%s, status = %d", esp_err_to_name(result), response_code);
    return false;
}