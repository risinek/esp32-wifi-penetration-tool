#ifndef DEAUTHER_H
#define DEAUTHER_H

#include "esp_wifi_types.h"

void deauther_send_deauth_frame(const wifi_ap_record_t *ap_record);

#endif