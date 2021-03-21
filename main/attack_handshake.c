#include "attack_handshake.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "attack.h"
#include "deauther.h"

static const char *TAG = "main:attack_handshake";

void attack_handshake_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting handshake attack...");
    deauther_send_deauth_frame();
}

void attack_handshake_stop(){
    ESP_LOGD(TAG, "Handshake attack stopped");
}