/**
 * @file attack.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-02
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements common attack wrapper.
 */

#include "attack.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "attack_pmkid.h"
#include "attack_handshake.h"
#include "attack_dos.h"
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
        ESP_LOGD(TAG, "Stopping attack timeout timer");
        ESP_ERROR_CHECK(esp_timer_stop(attack_timeout_handle));
    } 
}

void attack_append_status_content(uint8_t *buffer, unsigned size){
    if(size == 0){
        ESP_LOGE(TAG, "Size can't be 0 if you want to reallocate");
        return;
    }
    // temporarily save new location in case of realloc failure to preserve current content
    char *reallocated_content = realloc(attack_status.content, attack_status.content_size + size);
    if(reallocated_content == NULL){
        ESP_LOGE(TAG, "Error reallocating status content! Status content may not be complete.");
        return;
    }
    // copy new data after current content
    memcpy(&reallocated_content[attack_status.content_size], buffer, size);
    attack_status.content = reallocated_content;
    attack_status.content_size += size;
}

char *attack_alloc_result_content(unsigned size) {
    attack_status.content_size = size;
    attack_status.content = (char *) malloc(size);
    return attack_status.content;
}

/**
 * @brief Callback function for attack timeout timer.
 * 
 * This function is called when attack times out. 
 * It updates attack status state to TIMEOUT.
 * It calls appropriate abort functions based on current attack type.
 * @param arg not used.
 */
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
            attack_handshake_stop();
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGI(TAG, "Abort PASSIVE attack...");
            break;
        case ATTACK_TYPE_DOS:
            ESP_LOGI(TAG, "Abort DOS attack...");
            attack_dos_stop();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type. Not aborting anything");
    }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_REQUEST event.
 * 
 * This function handles WEBSERVER_EVENT_ATTACK_REQUEST event from event loop.
 * It parses attack_request_t structure and set initial values to attack_status.
 * It sets attack state to RUNNING.
 * It starts attack timeout timer.
 * It starts attack based on chosen type.
 * 
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_REQUEST
 * @param event_data expects attack_request_t
 */
static void attack_request_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Starting attack...");

    attack_request_t* attack_request = (attack_request_t*) event_data;
    ap_records_t ap_records;
    ap_records.len = attack_request->ap_records_len;
    ap_records.records = malloc(ap_records.len * sizeof(wifi_ap_record_t*));
    // attack_config.ap_record = wifictl_get_ap_record(attack_request->ap_record_id);
    for (int i = 0; i < ap_records.len; ++i) {
        ap_records.records[i] = wifictl_get_ap_record(attack_request->ap_records_ids[i]);
        if (ap_records.records[i] == NULL) {
            ESP_LOGE(TAG, "NPE: No ap_record for id '%d'!", attack_request->ap_records_ids[i]);
            free(ap_records.records);
            free(attack_request->ap_records_ids);
            return;
        }
    }
    free(attack_request->ap_records_ids);

    attack_config_t attack_config = {
      .type = attack_request->type,
      .method = attack_request->method,
      .timeout = attack_request->timeout,
      .ap_records = ap_records
    };

    attack_status.state = RUNNING;
    attack_status.type = attack_config.type;

    // if(attack_config.ap_record == NULL){
    //     ESP_LOGE(TAG, "NPE: No attack_config.ap_record!");
    //     return;
    // }

    




    // TODO(alambin):
    // 1. Implement Bluetooth PIN/password/passcode request.
    //    Currently we can reset device by writing characteristic, which is not so convenient.
    // 2. Update WebUI to support all new freatures:
    //    X 1. ATTACK_DOS_METHOD_BROADCAST_MULTI_AP
    //    2. ATTACK_TYPE_STOP_ATTACK
    //    3. If attack is DOS, then we can run it forever by setting timeout to 0.
    //       Should display info that RougeAP attack can be stopped only by reset (hard or soft by connecting to ESP-32
    //       via Bluetooth)
    //    4. On WebUI side instead of setting ap_record_id as 1st field, it should be represented as list of size 1
    // 3. We can track all clients which are trying to connect to WiFi APs. Then during broadcast attack on
    //    given AP we will send not only broadcast deauth (which is ignored by most of devices), but also
    //    deauth for all clients we observed. Not to blow up this list, we can clean it, say, once per day
    // 4. Need to make sure that all my changes are not breaking existing code. Ex. that proper status of attack will
    //    be returned by attack_get_status(), that each attack really has proper status (remember, that now we have
    //    infinite attacks and ability to interrupt attacks)
    // 5. Make sure that timeout in WebUI is handled well (even in case of infinite attack).
    



    // 5. Broadcast attack can be extended on multiple Access Points.
    //    We can introduce new method - ATTACK_DOS_METHOD_BROADCAST. If it is selected in WebUI, we can allow to
    //    select multiple APs. Then we send more data in HTTP Post request: ap_list_len, ap0, ap1, ...
    //    Sending is not a problem, but how to read data with various sise on ESp32 side?
    //    Now in uri_run_attack_post_handler() we are using httpd_req_recv, which reads only specified
    //    (sizeof(attack_request_t)) amount of bytes. In case we can call this method 2 times, then in 1st call we
    //    figure out size of list of APs and in 2nd call we can read this list (Answer - can NOT).
    //    Need either find approve in documentation, either try it with board.
    //    Idea: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
    //         "Use content_len provided in httpd_req_t structure to know the length of data to be fetched"
    //         So, attack_request_t should be
                // typedef struct {
                //     uint8_t type;
                //     uint8_t method;
                //     uint8_t timeout;
                //     uint8_t ap_record_len;
                //     uint8_t* ap_record_ids;
                // } attack_request_t;
    //
    // And code to read data should be
    // attack_request_t attack_request;
    // void *rawData = malloc(req->content_len);
    // attack_request_t* raw_attack_request = rawData;
    // httpd_req_recv(req, (char *)rawData, req->content_len);

    // attack_request.type = raw_attack_request.type;
    // attack_request.method = raw_attack_request.method;
    // attack_request.tytimeoute = raw_attack_request.timeout;
    // attack_request.ap_record_len = raw_attack_request.ap_record_len;
    // attack_request.ap_record_ids = malloc(attack_request.ap_record_len);

    // uint8_t* currentId = &(uint8_t)(raw_attack_request.ap_record_ids)
    // for (int i = 0; i < attack_request.ap_record_len; ++i) {
    //     attack_request.ap_record_ids[i] = *currentId;
    //     currentId++;
    // }
    //
    // Try this code FIRST to pass 1 value and print logs. If it works, extend to multiple values and make sure
    // by logs that it works

    // DO NOT FORGET TO FREE attack_request.ap_record_ids SOMEWHERE




    // Set timeout to stop attack.
    // In case it is DOS broadcast and timeout is 0, do not set timer and let attack to run forever
    // NOTE! In case of RougeAP attack, new AP will be established forever. The only way to stop it is reset: either
    // hard (button on board), either soft. Currently to trigger soft reset you need to connect to ESP-32 by Bluetooth
    if (((attack_config.timeout == 0) &&
         (attack_config.type == ATTACK_TYPE_DOS))) {
        ESP_LOGI(
            "Timeout is set to 0. Atack will not finish until reboot or "
            "ATTACK_TYPE_STOP_ATTACK command is received");
    } else {
        // Before starting timer, make sure previous attack is finished. If not
        // - finish it by simulating timeout
        esp_err_t stop_result = esp_timer_start_once(
            attack_timeout_handle, attack_config.timeout * 1000000);
        if (stop_result != ESP_OK) {
            if (stop_result == ESP_ERR_INVALID_STATE) {
              // Previous attack is still in progress. Try to stop it and start
              // timer again
              esp_timer_stop(attack_timeout_handle);
              // Simulate timeout event
              attack_timeout(NULL);

              ESP_ERROR_CHECK(esp_timer_start_once(
                  attack_timeout_handle, attack_config.timeout * 1000000));
            } else {
              ESP_ERROR_CHECK(stop_result);
            }
        }
    }

    // start attack based on it's type
    switch(attack_config.type) {
        case ATTACK_TYPE_PMKID:
            attack_pmkid_start(&attack_config);
            break;
        case ATTACK_TYPE_HANDSHAKE:
            attack_handshake_start(&attack_config);
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGW(TAG, "ATTACK_TYPE_PASSIVE not implemented yet!");
            break;
        case ATTACK_TYPE_DOS:
            attack_dos_start(&attack_config);
            break;
        case ATTACK_TYPE_STOP_ATTACK:
            // Stop timer if it was initiated by previous attack.
            esp_timer_stop(attack_timeout_handle);
            // Simulate timeout event. We should not call attack_update_status(FINISHED), because it will not finalize
            // attack properly (ex. it will not call attack_dos_stop(), etc.)
            attack_timeout(NULL);
            free(attack_config.ap_records.records);
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type!");
            free(attack_config.ap_records.records);
    }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_RESET event.
 * 
 * This callback resets attack status by freeing previously allocated status content and putting attack to READY state.
 * 
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_RESET
 * @param event_data not used
 */
static void attack_reset_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Resetting attack status...");
    if(attack_status.content){
        free(attack_status.content);
        attack_status.content = NULL;
    }
    attack_status.content_size = 0;
    attack_status.type = -1;
    attack_status.state = READY;
}

/**
 * @brief Initialises common attack resources.
 * 
 * Creates attack timeout timer.
 * Registers event loop event handlers.
 */
void attack_init(){
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}