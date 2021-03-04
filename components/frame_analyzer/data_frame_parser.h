#ifndef DATA_FRAME_PARSER_H
#define DATA_FRAME_PARSER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(DATA_FRAME_EVENTS);

enum {
    DATA_FRAME_EVENT_CAPTURED_EAPOLKEY
};

void parse_data_frame(wifi_promiscuous_pkt_t *frame);

#endif