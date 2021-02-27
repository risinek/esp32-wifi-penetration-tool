#include "frame_analyzer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "frame_analyzer"; 
void frame_analyzer_capture_wpa_handshake(){
    ESP_LOGI(TAG, "Capturing WPA handshake frames...");
}