// References: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"
#include "attack.h"

#include "pages/page_index.h"
#include "pages/page_result.h"

static const char* TAG = "webserver";
ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

static esp_err_t uri_root_get_handler(httpd_req_t *req) {
    return httpd_resp_send(req, page_index, HTTPD_RESP_USE_STRLEN);
}

static httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
};

static esp_err_t uri_ap_list_get_handler(httpd_req_t *req) {
    wifictl_scan_nearby_aps();

    const wifictl_ap_records_t *ap_records;
    ap_records = wifictl_get_ap_records();
    
    // 33 SSID + 6 BSSID + 1 RSSI
    char resp_chunk[40];
    
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    for(unsigned i = 0; i < ap_records->count; i++){
        memcpy(resp_chunk, ap_records->records[i].ssid, 33);
        memcpy(&resp_chunk[33], ap_records->records[i].bssid, 6);
        memcpy(&resp_chunk[39], &ap_records->records[i].rssi, 1);
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, resp_chunk, 40));
    }
    return httpd_resp_send_chunk(req, resp_chunk, 0);
}

static httpd_uri_t uri_ap_list_get = {
    .uri = "/ap-list",
    .method = HTTP_GET,
    .handler = uri_ap_list_get_handler,
    .user_ctx = NULL
};

static esp_err_t uri_run_attack_post_handler(httpd_req_t *req) {
    uint8_t config[3];
    httpd_req_recv(req, (char *)&config, 3);
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &config, sizeof(config), portMAX_DELAY));
    return httpd_resp_send(req, NULL, 0);
}

static httpd_uri_t uri_run_attack_post = {
    .uri = "/run-attack",
    .method = HTTP_POST,
    .handler = uri_run_attack_post_handler,
    .user_ctx = NULL
};

static esp_err_t uri_result_get_handler(httpd_req_t *req) {
    return httpd_resp_send(req, page_result, HTTPD_RESP_USE_STRLEN);
}

static httpd_uri_t uri_result_get = {
    .uri = "/result",
    .method = HTTP_GET,
    .handler = uri_result_get_handler,
    .user_ctx = NULL
};

static esp_err_t uri_get_result_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Fetching attack result...");
    const attack_result_t *attack_result;
    attack_result = attack_get_result();

    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    // first send attack result header
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (char *) attack_result, 3));
    // send attack result content
    if(attack_result->content_size > 0){
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, attack_result->content, attack_result->content_size));
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

static httpd_uri_t uri_get_result_get = {
    .uri = "/get-result",
    .method = HTTP_GET,
    .handler = uri_get_result_get_handler,
    .user_ctx = NULL
};

void webserver_run(){
    ESP_LOGD(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_list_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_run_attack_post));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_result_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_get_result_get));
}