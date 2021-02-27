#include "frame_analyzer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "wifi_controller.h"
#include "data_frame_parser.h"

static const char *TAG = "frame_analyzer";

static void data_frame_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Handling DATA frame");
    parse_data_frame(event_data);
}

void frame_analyzer_capture_wpa_handshake(){
    ESP_LOGI(TAG, "Capturing WPA handshake frames...");

    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, NULL));
}