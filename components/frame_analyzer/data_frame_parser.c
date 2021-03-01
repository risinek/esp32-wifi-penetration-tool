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
    wifi_promiscuous_pkt_t *pframe = (wifi_promiscuous_pkt_t *) frame;
    print_raw_frame(pframe);
}