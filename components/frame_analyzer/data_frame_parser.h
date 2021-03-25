#ifndef DATA_FRAME_PARSER_H
#define DATA_FRAME_PARSER_H

#include <stdint.h>
#include "esp_wifi_types.h"

#include "frame_analyzer_types.h"

wifi_promiscuous_pkt_t *filter_frame(wifi_promiscuous_pkt_t *frame, uint8_t *bssid);
eapol_packet_t *parse_eapol_packet(wifi_promiscuous_pkt_t *frame);
pmkid_item_t *parse_pmkid_from_eapol_packet(eapol_packet_t *eapol_packet);

#endif