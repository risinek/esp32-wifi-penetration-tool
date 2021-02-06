// References: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"

static const char* TAG = "webserver";

esp_err_t uri_get_handler(httpd_req_t *req) {
    const char resp[] = "<h1>Test!</h1>";
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

httpd_uri_t uri_get = {
    .uri = "/index",
    .method = HTTP_GET,
    .handler = uri_get_handler,
    .user_ctx = NULL
};

void webserver_run(){
    ESP_LOGD(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_get));

}