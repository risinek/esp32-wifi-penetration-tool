#ifndef AP_SCANNER_H
#define AP_SCANNER_H

#include "esp_wifi_types.h"

typedef struct {
    uint16_t count;
    wifi_ap_record_t records[CONFIG_SCAN_MAX_AP];
} wifictl_ap_records_t;


void wifictl_scan_nearby_aps();
const wifictl_ap_records_t *wifictl_get_ap_records();

#endif