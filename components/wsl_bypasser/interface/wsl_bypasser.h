/**
 * @file wsl_bypasser.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 *
 * @brief Provides interface for Wi-Fi Stack Libraries bypasser
 *
 * This component bypasses blocking mechanism that doesn't allow sending some arbitrary 802.11 frames like deauth etc.
 */
#ifndef WSL_BYPASSER_H
#define WSL_BYPASSER_H

#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sends broadcast deauthentication frame with forged source AP from given ap_record
 *
 * This will send deauthentication frame acting as frame from given AP, and destination will be broadcast
 * MAC address - \c ff:ff:ff:ff:ff:ff
 *
 * @param ap_record AP record with valid AP information
 */
void wsl_bypasser_send_broadcast_deauth_frame(const wifi_ap_record_t *ap_record);

/**
 * @brief The same version of function, as above, but it lets you to specify target MAC addresses. This increases
 * effectivenes of deauth attack, because we not only send deauth frame from AP to station, but also from station to AP
 */
void wsl_bypasser_send_deauth_frame_to_targets(const wifi_ap_record_t *ap_record, const uint8_t *destination_macs,
                                               uint16_t macs_num, const bool *isStopRequested);

#ifdef __cplusplus
}
#endif

#endif  // WSL_BYPASSER_H
