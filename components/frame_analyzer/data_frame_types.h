#ifndef DATA_FRAME_TYPES_H
#define DATA_FRAME_TYPES_H


typedef enum {
	ETHER_TYPE_EAPOL = 0x888e
} ether_types_t;

typedef enum {
    EAPOL_EAP_PACKET,
	EAPOL_START,
	EAPOL_LOGOFF,
	EAPOL_KEY,
	EAPOL_ENCAPSULATED_ASF_ALERT
} eapol_packet_types_t;

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

typedef struct {
	uint8_t version;
	eapol_packet_types_t packet_type:8;
	uint16_t packet_body_length;
} eapol_header_t;

typedef struct {
	eapol_header_t hdr;
	uint8_t payload[];
} eapol_packet_t;

#endif