#include "attack.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "attack_pmkid.h"

static const char* TAG = "attack";
static attack_result_t attack_result = { .status = IDLE };
// static attack_config_t attack_config = { .ap_record = NULL, .timeout  = 0, .type = PASSIVE};

const attack_result_t *attack_get_result() {
    return &attack_result;
}

void attack_run(attack_config_t attack_config) {
    ESP_LOGI(TAG, "Starting attack...");
    attack_result.status = RUNNING;

    if(attack_config.ap_record == NULL){
        ESP_LOGE(TAG, "NPE: No attack_config.ap_record!");
        return;
    }
    switch(attack_config.type) {
        case ATTACK_TYPE_PMKID:
            attack_pmkid(&attack_config);
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