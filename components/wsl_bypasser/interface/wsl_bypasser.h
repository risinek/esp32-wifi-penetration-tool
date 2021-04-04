#ifndef WSL_BYPASSER_H
#define WSL_BYPASSER_H

#include "esp_wifi_types.h"

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size);
void wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record);

#endif