#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(DATA_FRAME_EVENTS);

enum {
    DATA_FRAME_EVENT_CAPTURED_EAPOLKEY,
    DATA_FRAME_EVENT_FOUND_PMKID
};

void frame_analyzer_pmkid_capture_start(const uint8_t *bssid);
void frame_analyzer_pmkid_capture_stop();

#endif