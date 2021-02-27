#include "data_frame_parser.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_wifi.h"

const char *TAG = "frame_analyzer:data_frame_parser";

typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t sequence_ctrl;
    uint8_t addr4[6];
} data_frame_mac_header_t;

typedef struct {
    data_frame_mac_header_t header;
    uint16_t etherType;
    uint8_t payload[0];
} data_frame_t;

typedef enum {
	ETHER_TYPE_EAPOL = 0x888e
} ether_types_t;
void parse_data_frame(void *frame) {
    wifi_promiscuous_pkt_t *pframe = (wifi_promiscuous_pkt_t *) frame;
    data_frame_t *rframe = (data_frame_t *) pframe->payload;
}