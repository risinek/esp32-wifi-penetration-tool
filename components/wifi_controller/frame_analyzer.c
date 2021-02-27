#include "frame_analyzer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "frame_analyzer"; 

static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGD(TAG, "Captured frame.");
}

void wifictl_frame_analyzer_start() {
    ESP_LOGI(TAG, "Initialising frame analyzer...");
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
}