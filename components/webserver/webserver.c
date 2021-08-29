/**
 * @file webserver.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 *
 * @brief Implements Webserver component and all available enpoints.
 *
 * Webserver is built on esp_http_server subcomponent from ESP-IDF
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
 */
#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"
#include "attack.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"

#include "pages/page_index.h"

static const char* TAG = "webserver";
ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

/**
 * @brief Handlers for index/root \c / path endpoint
 *
 * This endpoint provides index page source
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)page_index, page_index_len);
}

static httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
};
//@}

/**
 * @brief Handlers for \c /reset endpoint
 *
 * This endpoint resets the attack logic to initial READY state.
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_reset_head_handler(httpd_req_t *req) {
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, NULL, 0, portMAX_DELAY));
    return httpd_resp_send(req, NULL, 0);
}

static httpd_uri_t uri_reset_head = {
    .uri = "/reset",
    .method = HTTP_HEAD,
    .handler = uri_reset_head_handler,
    .user_ctx = NULL
};
//@}

/**
 * @brief Handlers for \c /ap-list endpoint
 *
 * This endpoint returns list of available APs nearby.
 * It calls wifi_controller ap_scanner and serialize their SSIDs into octet response.
 * @attention reponse may take few seconds
 * @attention client may be disconnected from ESP AP after calling this endpoint
 * @param req
 * @return esp_err_t
 * @{
 */
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
//@}

/**
 * @brief Handlers for \c /run-attack endpoint
 *
 * This endpoint receives attack configuration from client. It deserialize it from octet stream to attack_request_t structure.
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_run_attack_post_handler(httpd_req_t *req) {
    attack_request_t attack_request;
    httpd_req_recv(req, (char *)&attack_request, sizeof(attack_request_t));
    esp_err_t res = httpd_resp_send(req, NULL, 0);
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request, sizeof(attack_request_t), portMAX_DELAY));
    return res;
}

static httpd_uri_t uri_run_attack_post = {
    .uri = "/run-attack",
    .method = HTTP_POST,
    .handler = uri_run_attack_post_handler,
    .user_ctx = NULL
};
//@}

/**
 * @brief Handlers for \c /status endpoint
 *
 * This endpoint fetches current status from main component attack wrapper, serialize it and sends it to client as octet stream.
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_status_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Fetching attack status...");
    const attack_status_t *attack_status;
    attack_status = attack_get_status();

    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    // first send attack result header
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (char *) attack_status, 4));
    // send attack result content
    if(((attack_status->state == FINISHED) || (attack_status->state == TIMEOUT)) && (attack_status->content_size > 0)){
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, attack_status->content, attack_status->content_size));
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

static httpd_uri_t uri_status_get = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = uri_status_get_handler,
    .user_ctx = NULL
};
//@}

/**
 * @brief Handlers for \c /capture.pcap endpoint
 *
 * This endpoint forwards PCAP binary data from pcap_serializer via octet stream to client.
 *
 * @note Most browsers will start download process when this endpoint is called.
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_capture_pcap_get_handler(httpd_req_t *req){
    ESP_LOGD(TAG, "Providing PCAP file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char *) pcap_serializer_get_buffer(), pcap_serializer_get_size());
}

static httpd_uri_t uri_capture_pcap_get = {
    .uri = "/capture.pcap",
    .method = HTTP_GET,
    .handler = uri_capture_pcap_get_handler,
    .user_ctx = NULL
};
//@}

/**
 * @brief Handlers for \c /capture.hccapx endpoint
 *
 * This endpoint forwards HCCAPX binary data from hccapx_serializer via octet stream to client.
 *
 * @note Most browsers will start download process when this endpoint is called.
 * @param req
 * @return esp_err_t
 * @{
 */
static esp_err_t uri_capture_hccapx_get_handler(httpd_req_t *req){
    ESP_LOGD(TAG, "Providing HCCAPX file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char *) hccapx_serializer_get(), sizeof(hccapx_t));
}

static httpd_uri_t uri_capture_hccapx_get = {
    .uri = "/capture.hccapx",
    .method = HTTP_GET,
    .handler = uri_capture_hccapx_get_handler,
    .user_ctx = NULL
};
//@}

void webserver_run(){
    ESP_LOGD(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_reset_head));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_list_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_run_attack_post));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_status_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_pcap_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_hccapx_get));
}