#include "data_frame_parser.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_wifi.h"

#include "data_frame_types.h"

const char *TAG = "frame_analyzer:data_frame_parser";

void print_raw_frame(wifi_promiscuous_pkt_t *frame){
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++) {
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
}

void print_mac_address(uint8_t *a){
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
    a[0], a[1], a[2], a[3], a[4], a[5]);
}

void parse_data_frame(void *frame) {
    uint8_t *frame_buffer = ((wifi_promiscuous_pkt_t *) frame)->payload;

    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame_buffer;
    frame_buffer += sizeof(data_frame_mac_header_t);


    if((mac_header->addr1[0] != 0x04) && (mac_header->addr2[0] != 0x04)) {
        return;
    }

    if(mac_header->frame_control.protected_frame == 1) {
        ESP_LOGV(TAG, "Protected frame, skipping...");
        return;
    }

    if(mac_header->frame_control.subtype > 7) {
        ESP_LOGD(TAG, "QoS data frame");
        // Skipping QoS field (2 bytes)
        frame_buffer += 2;
    }

    // Skipping LLC SNAP header (6 bytes)
    frame_buffer += sizeof(llc_snap_header_t);

    print_mac_address(mac_header->addr1);
    printf("; ");
    print_mac_address(mac_header->addr2);
    printf("\n");
    print_raw_frame(frame);

    // Check if frame_data is type of EAPoL
    if(ntohs(*(uint16_t *) frame_buffer) == ETHER_TYPE_EAPOL) {
        frame_buffer += 2;
        ESP_LOGD(TAG, "EAPOL packet");
        eapol_packet_header_t *eapol_packet_header = (eapol_packet_header_t *) frame_buffer;
        frame_buffer += sizeof(eapol_packet_header_t);
        if(eapol_packet_header->packet_type == EAPOL_KEY) {
            ESP_LOGD(TAG, "EAPOL-Key");
        }
    }
}