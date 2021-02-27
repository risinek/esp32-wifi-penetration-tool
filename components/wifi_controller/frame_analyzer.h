#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include <stdbool.h>

void wifictl_frame_analyzer_filter_frame_types(bool data, bool mgmt, bool ctrl);
void wifictl_frame_analyzer_start();
void wifictl_frame_analyzer_stop();

#endif