#ifndef WIFI_FRAME_SENDER_H
#define WIFI_FRAME_SENDER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_wifi_frame_sender();

/**
 * @brief Sends frame in frame_buffer using esp_wifi_80211_tx but bypasses blocking mechanism
 *
 * @param frame_buffer
 * @param size size of frame buffer
 */
void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif  // WIFI_FRAME_SENDER_H