#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(DATA_FRAME_EVENTS);

enum {
    DATA_FRAME_EVENT_CAPTURED_EAPOLKEY
};

void frame_analyzer_capture_wpa_handshake();

#endif