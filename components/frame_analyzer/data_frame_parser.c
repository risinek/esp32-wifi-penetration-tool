#include "data_frame_parser.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_wifi.h"

#include "data_frame_types.h"

const char *TAG = "frame_analyzer:data_frame_parser";


void parse_data_frame(void *frame) {
    wifi_promiscuous_pkt_t *pframe = (wifi_promiscuous_pkt_t *) frame;
    data_frame_t *rframe = (data_frame_t *) pframe->payload;
    ESP_LOGD(TAG, "Got %02x", rframe->header.frame_ctrl);
}