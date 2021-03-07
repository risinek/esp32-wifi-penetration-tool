#ifndef DATA_FRAME_TYPES_H
#define DATA_FRAME_TYPES_H

#include <stdint.h>

// Ref: 802.1X-2020 [11.1.4]
#define ETHER_TYPE_EAPOL 0x888e

// Ref: 802.1X-2020 [11.3.2]
typedef enum {
    EAPOL_EAP_PACKET = 0,
	EAPOL_START,
	EAPOL_LOGOFF,
	EAPOL_KEY,
	EAPOL_ENCAPSULATED_ASF_ALERT,
    EAPOL_MKA,
    EAPOL_ANNOUNCEMENT_GENERIC,
    EAPOL_ANNOUNCEMENT_SPECIFIC,
    EAPOL_ANNOUNCEMENT_REQ
} eapol_packet_types_t;

typedef struct {
    uint8_t protocol_version:2;
    uint8_t type:2;
    uint8_t subtype:4;
    uint8_t to_ds:1;
    uint8_t from_ds:1;
    uint8_t more_fragments:1;
    uint8_t retry:1;
    uint8_t power_management:1;
    uint8_t more_data:1;
    uint8_t protected_frame:1;
    uint8_t htc_order:1;
} frame_control_t;

typedef struct {
    frame_control_t frame_control;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t sequence_control;
} data_frame_mac_header_t;

typedef struct {
    uint8_t snap_dsap;
    uint8_t snap_ssap;
    uint8_t control;
    uint8_t encapsulation[3];
} llc_snap_header_t;

// Ref: 802.1X-2020 [11.3]
typedef struct {
	uint8_t version;
	uint8_t packet_type;
	uint16_t packet_body_length;
} eapol_packet_header_t;

// Ref: 802.1X-2020 [11.3], 802.11-2016 [12.7.2]
typedef struct {
	eapol_packet_header_t header;
	uint8_t packet_body[];
} eapol_packet_t;

// Ref: 802.11-2016 [12.7.2]
// unnamed fields are "reserved"
typedef struct {
    uint8_t key_descriptor_version:3;
    uint8_t key_type:1;
    uint8_t :2;
    uint8_t install:1;
    uint8_t key_ack:1;
    uint8_t key_mic:1;
    uint8_t secure:1;
    uint8_t error:1;
    uint8_t request:1;
    uint8_t encrypted_key_data:1;
    uint8_t smk_message:1;
    uint8_t :2;
} key_information_t;

// Ref: 802.11-2016 [12.7.2]
typedef struct __attribute__((__packed__)) {
    uint8_t descriptor_type;
    key_information_t key_information;
    uint16_t key_length;
    uint8_t key_replay_counter[8];
    uint8_t key_nonce[32];
    uint8_t key_iv[16];
    uint8_t key_rsc[8];
    uint8_t reserved[8];
    // TODO MIC lenght is dependent on 802.11-2016 [12.7.3] - key_infromation_t.descrptor_version
    uint8_t key_mic[16];
    uint16_t key_data_length;
    uint8_t key_data[];
} eapol_key_packet_t;

// Ref: 802.11-2016 [12.7.2]
typedef struct __attribute__((__packed__)) {
    uint8_t type;
    uint8_t length;
    uint32_t oui:24;
    uint32_t data_type:8;
    uint8_t data[];
} key_data_field_t;

#endif