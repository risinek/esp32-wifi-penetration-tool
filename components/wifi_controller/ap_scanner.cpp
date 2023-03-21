/**
 * @file ap_scanner.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 *
 * @brief Implements AP scanning functionality.
 */
#include "ap_scanner.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <cstring>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"

namespace {
const char* TAG = "wifi_controller/ap_scanner";
}  // namespace

bool are_macs_equal(const uint8_t* l, const uint8_t* r) { return (memcmp(l, r, 6) == 0); }

/**
 * @brief Stores last scanned AP records into linked list.
 *
 */
static wifictl_ap_records_t ap_records;

void wifictl_scan_nearby_aps() {
  ESP_LOGD(TAG, "Scanning nearby APs...");

  ap_records.clear();
  wifi_scan_config_t scan_config{};
  scan_config.ssid = NULL;
  scan_config.bssid = NULL;
  scan_config.channel = 0;
  scan_config.show_hidden = false;
  scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

  uint16_t numOfApFound{CONFIG_SCAN_MAX_AP};
  std::vector<wifi_ap_record_t> tmpApRecords;
  tmpApRecords.resize(CONFIG_SCAN_MAX_AP);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&numOfApFound, tmpApRecords.data()));
  for (int i = 0; i < numOfApFound; ++i) {
    ap_records.push_back(std::make_shared<const wifi_ap_record_t>(tmpApRecords[i]));  // Create shared_ptr with copy
  }
  tmpApRecords.clear();

  ESP_LOGI(TAG, "Found %u APs.", ap_records.size());
  ESP_LOGD(TAG, "Scan done.");
}

const wifictl_ap_records_t& wifictl_get_ap_records() { return ap_records; }

std::shared_ptr<const wifi_ap_record_t> wifictl_get_ap_record(unsigned index) {
  if (index > ap_records.size()) {
    ESP_LOGE(TAG, "Index out of bounds! %u records available, but %u requested", ap_records.size(), index);
    return NULL;
  }
  return ap_records[index];
}

std::shared_ptr<const wifi_ap_record_t> wifictl_get_ap_record_by_mac(const uint8_t* mac) {
  for (uint16_t i = 0; i < ap_records.size(); ++i) {
    if (are_macs_equal(ap_records[i]->bssid, mac)) {
      return ap_records[i];
    }
  }
  return nullptr;
}
