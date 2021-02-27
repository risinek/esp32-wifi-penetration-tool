#include "frame_analyzer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "frame_analyzer"; 

void wifictl_frame_analyzer_start() {
    ESP_LOGI(TAG, "Initialising frame_analyzer component...");
}