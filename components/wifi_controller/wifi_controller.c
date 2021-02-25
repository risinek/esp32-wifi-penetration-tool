#include "wifi_controller.h"

#include <stdio.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char* TAG = "wifi_controller";

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

}

static void wifi_init_apsta(){
    if (esp_wifi_get_mode(NULL) == ESP_OK) {
        ESP_LOGD(TAG, "APSTA already initialised. Skipping.");
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());
    
}

void wifictl_ap_start(wifi_config_t *wifi_config) {
    ESP_LOGD(TAG, "Starting AP...");
    
    wifi_init_apsta();
    
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, wifi_config));

    ESP_LOGI(TAG, "AP started with SSID=%s", wifi_config->ap.ssid);
}

void wifictl_mgmt_ap_start(){
    wifi_config_t mgmt_wifi_config = {
        .ap = {
            .ssid = CONFIG_MGMT_AP_SSID,
            .ssid_len = strlen(CONFIG_MGMT_AP_SSID),
            .channel = CONFIG_MGMT_AP_CHANNEL,
            .password = CONFIG_MGMT_AP_PASSWORD,
            .max_connection = CONFIG_MGMT_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    wifictl_ap_start(&mgmt_wifi_config);
}