#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include <stdbool.h>

void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);
void wifictl_sniffer_start();
void wifictl_sniffer_stop();

#endif