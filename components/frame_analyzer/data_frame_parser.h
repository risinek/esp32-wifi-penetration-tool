#ifndef DATA_FRAME_PARSER_H
#define DATA_FRAME_PARSER_H

#include "esp_wifi_types.h"

#include "data_frame_types.h"

typedef struct {
    const uint8_t *bssid;
} frame_filter_t;

typedef struct pmkid_item {
    uint8_t pmkid[16];
    struct pmkid_item *next;
} pmkid_item_t;

wifi_promiscuous_pkt_t *filter_frame(wifi_promiscuous_pkt_t *frame, frame_filter_t *filter);
eapol_packet_t *parse_eapol_packet(wifi_promiscuous_pkt_t *frame);
pmkid_item_t *parse_pmkid_from_eapol_packet(eapol_packet_t *eapol_packet);

#endif