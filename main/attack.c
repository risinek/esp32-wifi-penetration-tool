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
static attack_status_t attack_status = { .state = READY, .type = -1, .content_size = 0, .content = NULL };
static esp_timer_handle_t attack_timeout_handle;

const attack_status_t *attack_get_status() {
    return &attack_status;
}

void attack_update_status(attack_state_t state) {
    attack_status.state = state;
    if(state == FINISHED) {
        // Stop timeout timer
        ESP_LOGD(TAG, "Stopping attack timeout timer");
        ESP_ERROR_CHECK(esp_timer_stop(attack_timeout_handle));
    } 
}

char *attack_alloc_result_content(unsigned size) {
    attack_status.content_size = size;
    attack_status.content = (char *) malloc(size);
    return attack_status.content;
}

static void attack_timeout(void* arg){
    ESP_LOGD(TAG, "Attack timed out");
    
    attack_update_status(TIMEOUT);

    switch(attack_status.type) {
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
    attack_request_t *attack_request = (attack_request_t *) event_data;
    attack_config_t attack_config = { .type = attack_request->type, .timeout = attack_request->timeout };
    attack_config.ap_record = wifictl_get_ap_record(attack_request->ap_record_id);
    
    attack_status.state = RUNNING;
    attack_status.type = attack_config.type;

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
            ESP_LOGE(TAG, "Unknown attack type!");
    }
}

static void attack_reset_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Resetting attack status...");
    if(attack_status.content){
        free(attack_status.content);
    }
    attack_status.content_size = 0;
    attack_status.type = -1;
    attack_status.state = READY;
}

void attack_init(){
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}