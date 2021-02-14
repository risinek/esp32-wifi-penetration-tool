#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "esp_wifi_types.h"

typedef struct {
    uint16_t count;
    wifi_ap_record_t records[CONFIG_SCAN_MAX_AP];
} wifictl_ap_records_t;

void wifictl_mgmt_ap_start();
void wifictl_scan_nearby_aps();
wifictl_ap_records_t *wifictl_get_ap_records();

#endif