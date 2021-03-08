#include "attack.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "wifi_controller.h"
#include "frame_analyzer.h"

static const char* TAG = "attack";
static attack_result_t attack_result = { .status = IDLE };

const attack_result_t *attack_get_result() {
    return &attack_result;
}

void attack_run(const attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting attack...");
    attack_result.status = RUNNING;

    if(attack_config->ap_record == NULL){
        ESP_LOGE(TAG, "NPE: No attack_config->ap_record!");
        return;
    }
    switch(attack_config->type) {
        case ATTACK_TYPE_PMKID:
            ESP_LOGI(TAG, "Attack on PMKID...");
            wifictl_sniffer_filter_frame_types(true, false, false);
            wifictl_sniffer_start(attack_config->ap_record->primary);
            frame_analyzer_capture_pmkid(attack_config->ap_record->bssid);
            // connect as STA to AP
            break;
        case ATTACK_TYPE_HANDSHAKE:
            ESP_LOGI(TAG, "Attack on WPA handshake...");
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGI(TAG, "Passive attack with timeout...");
            break;
        default:
            ESP_LOGE(TAG, "Uknown attack type!");
    }
}