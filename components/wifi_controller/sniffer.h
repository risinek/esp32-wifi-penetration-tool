#ifndef SNIFFER_H
#define SNIFFER_H

#include <stdbool.h>
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL
};

void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);
void wifictl_sniffer_start();
void wifictl_sniffer_stop();

#endif