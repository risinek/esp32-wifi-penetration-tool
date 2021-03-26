#ifndef HCCAPX_SERIALIZER_H
#define HCCAPX_SERIALIZER_H

#include <stdint.h>

#include "frame_analyzer_types.h"

// Ref: https://hashcat.net/wiki/doku.php?id=hccapx
typedef struct __attribute__((__packed__)){
    uint32_t signature;
    uint32_t version;
    uint8_t message_pair;
    uint8_t essid_len;
    uint8_t essid[32];
    uint8_t keyver;
    uint8_t keymic[16];
    uint8_t mac_ap[6];
    uint8_t nonce_ap[32];
    uint8_t mac_sta[6];
    uint8_t nonce_sta[32];
    uint16_t eapol_len;
    uint8_t eapol[256];
} hccapx_t;

void hccapx_serializer_init(uint8_t *ssid, unsigned size);
hccapx_t *hccapx_serializer_get();
void hccapx_serializer_add_frame(data_frame_t *frame, unsigned size);

#endif