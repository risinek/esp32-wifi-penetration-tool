#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <stdint.h>

#include "../ap_scanner.h"
#include "../sniffer.h"

#include "esp_wifi_types.h"

void wifictl_ap_start(wifi_config_t *wifi_config);
void wifictl_ap_stop();
void wifictl_mgmt_ap_start();
void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);
void wifictl_sta_disconnect();
void wifictl_set_ap_mac(const uint8_t *mac_ap);
void wifictl_get_sta_mac(uint8_t *mac_sta);

#endif