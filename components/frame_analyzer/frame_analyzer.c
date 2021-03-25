#include "frame_analyzer.h"

#include <stdint.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "wifi_controller.h"
#include "data_frame_parser.h"

static const char *TAG = "frame_analyzer";
static uint8_t target_bssid[6];
static search_type_t search_type = -1;

static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGV(TAG, "Handling DATA frame");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if(!is_frame_bssid_matching(frame, target_bssid)){
        ESP_LOGV(TAG, "Frame filtered out. Not matching BSSIDs.");
        return;
    }

    eapol_packet_t *eapol_packet = parse_eapol_packet(frame);
    if(eapol_packet == NULL){
        ESP_LOGV(TAG, "Not an EAPOL packet.");
        return;
    }

    if(!is_eapol_key_packet(eapol_packet)){
        ESP_LOGV(TAG, "Not an EAPOL-Key packet");
        return;
    }

    if(search_type == SEARCH_HANDSHAKE){
        ESP_ERROR_CHECK(esp_event_post(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, frame, sizeof(wifi_promiscuous_pkt_t) + frame->rx_ctrl.sig_len, portMAX_DELAY));
        return;
    }

    if(search_type == SEARCH_PMKID){
        pmkid_item_t *pmkid_items;
        if((pmkid_items = parse_pmkid_from_eapol_packet(eapol_packet)) == NULL){
            return;
        }
        ESP_ERROR_CHECK(esp_event_post(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_items, sizeof(pmkid_item_t *), portMAX_DELAY));
        return;
    }
}

void frame_analyzer_capture_start(search_type_t search_type_arg, const uint8_t *bssid){
    ESP_LOGI(TAG, "Frame analysis started...");
    search_type = search_type_arg;
    memcpy(&target_bssid, bssid, 6);
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, NULL));
}

void frame_analyzer_capture_stop(){
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &data_frame_handler));
}
