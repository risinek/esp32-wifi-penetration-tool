// References: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"

#include "include/pages.h"


static const char* TAG = "webserver";

esp_err_t uri_root_get_handler(httpd_req_t *req) {
    return httpd_resp_send(req, page_root, HTTPD_RESP_USE_STRLEN);
}

httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
};

esp_err_t uri_aps_get_handler(httpd_req_t *req) {
    uint16_t ap_max_count = 20;
    wifi_ap_record_t ap_records[ap_max_count];

    wifictl_scan_nearby_aps(&ap_max_count, ap_records);
    
    ESP_LOGI(TAG, "Got %u APs.", ap_max_count);
    for(unsigned i = 0; i < ap_max_count; i++){
        ESP_LOGD(TAG, "AP#%u: %s", i, ap_records[i].ssid);
    }

    const char resp_ok[] = "OK";
    return httpd_resp_send(req, resp_ok, HTTPD_RESP_USE_STRLEN);
}

httpd_uri_t uri_aps_get = {
    .uri = "/aps",
    .method = HTTP_GET,
    .handler = uri_aps_get_handler,
    .user_ctx = NULL
};

void webserver_run(){
    ESP_LOGD(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_aps_get));
}