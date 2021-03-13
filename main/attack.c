#include "attack.h"

#include <unistd.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "attack_pmkid.h"

static const char* TAG = "attack";
static attack_result_t attack_result = { .status = IDLE };

const attack_result_t *attack_get_result() {
    return &attack_result;
}

void attack_set_result(attack_status_t status) {
    attack_result.status = status;
}

static bool attack_timeout(unsigned seconds){
    sleep(seconds);
    if(attack_result.status == FINISHED) {
        ESP_LOGD(TAG, "Attack already finished. Not doing anything...");
        return false;
    }
    return true;
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
            attack_pmkid_start(&attack_config);
            if(attack_timeout(5)){
                ESP_LOGD(TAG, "Aborting attack!");
                attack_set_result(TIMEOUT);
                attack_pmkid_stop();
            }
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