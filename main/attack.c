#include "attack.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* TAG = "attack";
static attack_result_t attack_result = { .status = IDLE };

const attack_result_t *attack_get_result() {
    return &attack_result;
}

void attack_run(const attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting attack...");
    attack_result.status = RUNNING;
}