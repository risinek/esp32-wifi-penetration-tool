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

static void wifi_init(){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

}

static void wifi_init_ap(){
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Test1",
            .ssid_len = strlen("Test1"),
            .channel = 3,
            .password = "testtest",
            .max_connection = 2,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
}

void wifi_ctl_ap_start(){
    ESP_LOGD(TAG, "starting wifi AP");

    wifi_init();
    wifi_init_ap();

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started with SSID=%s", "Test1");
}