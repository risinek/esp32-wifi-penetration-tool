#include "wifi_controller.h"

#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* TAG = "wifi_controller";

void wifi_start_ap(){
    ESP_LOGD(TAG, "starting wifi AP");
}