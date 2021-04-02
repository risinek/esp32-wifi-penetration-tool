#ifndef DATA_FRAME_PARSER_H
#define DATA_FRAME_PARSER_H

#include <stdint.h>
#include "esp_wifi_types.h"

#include "frame_analyzer_types.h"

bool is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid);
eapol_packet_t *parse_eapol_packet(data_frame_t *frame);
eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet);
pmkid_item_t *parse_pmkid(eapol_key_packet_t *eapol_key);

#endif