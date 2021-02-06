#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "wifi_controller.h"
#include "webserver.h"

static const char* TAG = "main";

void app_main(void)
{
    ESP_LOGD(TAG, "app_main started");
    wifi_ctl_mgmt_ap_start();
    webserver_run();
}
