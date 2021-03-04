#ifndef DATA_FRAME_PARSER_H
#define DATA_FRAME_PARSER_H

#include "esp_wifi_types.h"

uint8_t *parse_eapol_packet(wifi_promiscuous_pkt_t *frame);

#endif