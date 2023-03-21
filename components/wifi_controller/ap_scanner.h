/**
 * @file ap_scanner.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 *
 * @brief Provides an interface for AP scanning functionality.
 */
#ifndef AP_SCANNER_H
#define AP_SCANNER_H

#include <memory>
#include <vector>

#include "esp_wifi_types.h"

using wifictl_ap_records_t = std::vector<std::shared_ptr<const wifi_ap_record_t>>;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Switches ESP into scanning mode and stores result.
 *
 */
void wifictl_scan_nearby_aps();

/**
 * @brief Returns current list of scanned APs.
 *
 * @return const wifictl_ap_records_t*
 */
const wifictl_ap_records_t& wifictl_get_ap_records();

/**
 * @brief Returns AP record on given index
 *
 * @param index
 * @return const wifi_ap_record_t*
 */
std::shared_ptr<const wifi_ap_record_t> wifictl_get_ap_record(unsigned index);

std::shared_ptr<const wifi_ap_record_t> wifictl_get_ap_record_by_mac(const uint8_t* mac);

#ifdef __cplusplus
}
#endif

#endif  // AP_SCANNER_H
