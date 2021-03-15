#include "attack.h"

#include <stdlib.h>
#include <unistd.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "attack_pmkid.h"
#include "webserver.h"
#include "wifi_controller.h"

static const char* TAG = "attack";
static attack_result_t attack_result = { .status = READY, .type = -1, .content_size = 0, .content = NULL };
static esp_timer_handle_t attack_timeout_handle;

const attack_result_t *attack_get_result() {
    return &attack_result;
}

void attack_set_result(attack_status_t status) {
    attack_result.status = status;
    if(status == FINISHED) {
        // Stop timeout timer
        ESP_LOGD(TAG, "Stopping attack timeout timer");
        ESP_ERROR_CHECK(esp_timer_stop(attack_timeout_handle));
    } 
}

char *attack_alloc_result_content(unsigned size) {
    attack_result.content_size = size;
    attack_result.content = (char *) malloc(size);
    return attack_result.content;
}

static void attack_timeout(void* arg){
    ESP_LOGD(TAG, "Attack timed out");
    
    attack_set_result(TIMEOUT);

    switch(attack_result.type) {
        case ATTACK_TYPE_PMKID:
            ESP_LOGI(TAG, "Aborting PMKID attack...");
            attack_pmkid_stop();
            break;
        case ATTACK_TYPE_HANDSHAKE:
            ESP_LOGI(TAG, "Abort HANDSHAKE attack...");
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGI(TAG, "Abort PASSIVE attack...");
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type. Not aborting anything");
    }
}

static void attack_request_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Starting attack...");
    uint8_t *config = (uint8_t *) event_data;
    attack_config_t attack_config = { .type = config[1], .timeout = config[2] };
    attack_config.ap_record = wifictl_get_ap_record(config[0]);
    
    attack_result.status = RUNNING;
    attack_result.type = attack_config.type;

    if(attack_config.ap_record == NULL){
        ESP_LOGE(TAG, "NPE: No attack_config.ap_record!");
        return;
    }
    // set timeout
    ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, attack_config.timeout * 1000000));
    // start attack based on it's type
    switch(attack_config.type) {
        case ATTACK_TYPE_PMKID:
            attack_pmkid_start(&attack_config);
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

void attack_init(){
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
}