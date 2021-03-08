#include "frame_analyzer.h"

#include <stdint.h>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "wifi_controller.h"
#include "data_frame_parser.h"

static const char *TAG = "frame_analyzer";

static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGV(TAG, "Handling DATA frame");
    frame_filter_t *frame_filter = (frame_filter_t *) args;
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if(filter_frame(frame, frame_filter) == NULL){
        ESP_LOGV(TAG, "Filtered out");
        return;
    }

    eapol_packet_t *eapol_packet;
    if((eapol_packet = parse_eapol_packet(frame)) != NULL){
        ESP_ERROR_CHECK(esp_event_post(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_CAPTURED_EAPOLKEY, frame->payload, frame->rx_ctrl.sig_len, portMAX_DELAY));
        pmkid_item_t *pmkid_items;
        if((pmkid_items = parse_pmkid_from_eapol_packet(eapol_packet)) != NULL){
            ESP_ERROR_CHECK(esp_event_post(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_FOUND_PMKID, pmkid_items, sizeof(pmkid_item_t), portMAX_DELAY));
        }
    }
}

void frame_analyzer_capture_pmkid(const uint8_t *bssid){
    ESP_LOGI(TAG, "Capturing PMKIDs in WPA handhshake...");
    frame_filter_t frame_filter = { .bssid = bssid };
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, &frame_filter));
}