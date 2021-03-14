#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "../ap_scanner.h"
#include "../sniffer.h"

#include "esp_wifi_types.h"

void wifictl_ap_start(wifi_config_t *wifi_config);
void wifictl_mgmt_ap_start();
void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);
void wifictl_sta_disconnect();

#endif