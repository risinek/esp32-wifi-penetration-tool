/**
 * @file sniffer.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements sniffer logic.
 */
#include "sniffer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "sniffer"; 

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

/**
 * @brief Callback for promiscuous reciever. 
 * 
 * It forwards captured frames into event pool and sorts them based on their type
 * - Data
 * - Management
 * - Control
 * 
 * @param buf 
 * @param type 
 */
static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGV(TAG, "Captured frame %d.", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;

    int32_t event_id;
    switch (type) {
        case WIFI_PKT_DATA:
            event_id = SNIFFER_EVENT_CAPTURED_DATA;
            break;
        case WIFI_PKT_MGMT:
            event_id = SNIFFER_EVENT_CAPTURED_MGMT;
            break;
        case WIFI_PKT_CTRL:
            event_id = SNIFFER_EVENT_CAPTURED_CTRL;
            break;
        default:
            return;
    }

    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id, frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), portMAX_DELAY));
}

/**
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv425wifi_promiscuous_filter_t
 */
void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    if(data) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    }
    else if(mgmt) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    }
    else if(ctrl) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    }
    esp_wifi_set_promiscuous_filter(&filter);
}

void wifictl_sniffer_start(uint8_t channel) {
    ESP_LOGI(TAG, "Starting promiscuous mode...");
    // ESP32 cannot switch port, if there is some STA connected to AP
    ESP_LOGD(TAG, "Kicking all connected STAs from AP");
    ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
}

void wifictl_sniffer_stop() {
    ESP_LOGI(TAG, "Stopping promiscuous mode...");
    esp_wifi_set_promiscuous(false);
}