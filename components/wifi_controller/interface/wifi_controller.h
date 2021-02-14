#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "../ap_scanner.h"

#include "esp_wifi_types.h"

void wifictl_ap_start(wifi_config_t *wifi_config);
void wifictl_mgmt_ap_start();

#endif