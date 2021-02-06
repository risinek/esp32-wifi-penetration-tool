#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "esp_wifi_types.h"

void wifictl_mgmt_ap_start();
void wifictl_scan_nearby_aps(uint16_t *ap_max_count, wifi_ap_record_t *ap_records);

#endif