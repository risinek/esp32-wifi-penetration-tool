/**
 * @file hccapx_serializer.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements HCCAPX serializer
 */
#include "hccapx_serializer.h"

#include <stdint.h>
#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "frame_analyzer_parser.h"

/**
 * @brief Constants based on reference
 * 
 * @see Ref: https://hashcat.net/wiki/doku.php?id=hccapx
 */
//@{
#define HCCAPX_SIGNATURE 0x58504348
#define HCCAPX_VERSION 4
#define HCCAPX_KEYVER_WPA 1
#define HCCAPX_KEYVER_WPA2 2
#define HCCAPX_MAX_EAPOL_SIZE 256
//@}

static char *TAG = "hccapx_serializer";

/**
 * @brief Default values for hccapx buffer
 */
static hccapx_t hccapx = { 
    .signature = HCCAPX_SIGNATURE, 
    .version = 4, 
    .message_pair = 255,
    .keyver = HCCAPX_KEYVER_WPA2
};

/**
 * @brief Stores last processed message
 */
//@{
static unsigned message_ap = 0;
static unsigned message_sta = 0;
//@}

/**
 * @brief Stores number of message from which was the EAPoL packet saved.  
 */
static unsigned eapol_source = 0;

/**
 * @brief Says whether array contains only zero values or not
 * 
 * @param array array pointer
 * @param size size of given array
 * @return true all values are zero
 * @return false some value is different from zero
 */
static bool is_array_zero(uint8_t *array, unsigned size){
    for(unsigned i = 0; i < size; i++){
        if(array[i] != 0){
            return false;
        }
    }
    return true;
}

void hccapx_serializer_init(const uint8_t *ssid, unsigned size){
    hccapx.essid_len = size;
    memcpy(hccapx.essid, ssid, size);
    hccapx.message_pair = 255;
}

hccapx_t *hccapx_serializer_get(){
    if(hccapx.message_pair == 255){
        return NULL;
    }
    
    return &hccapx;
}

/**
 * @brief Saves EAPoL-Key frame into HCCAPX buffer
 * 
 * Also sets Key MIC value to the one present in the given EAPoL-Key packet
 * 
 * @param eapol_packet EAPoL packet to be saved that includes also EAPoL header
 * @param eapol_key_packet EAPoL-Key parsed to get key MIC from it
 * @return unsigned
 * @return 1 if error occured
 * @return 0 if successfully saved
 */
static unsigned save_eapol(eapol_packet_t *eapol_packet, eapol_key_packet_t *eapol_key_packet){
    unsigned eapol_len = 0;
    eapol_len = sizeof(eapol_packet_header_t) + ntohs(eapol_packet->header.packet_body_length);
    if(eapol_len > HCCAPX_MAX_EAPOL_SIZE){
        ESP_LOGW(TAG, "EAPoL is too long (%u/%u)", eapol_len, HCCAPX_MAX_EAPOL_SIZE);
        return 1;
    }
    hccapx.eapol_len = eapol_len;
    memcpy(hccapx.eapol, eapol_packet, hccapx.eapol_len);
    memcpy(hccapx.keymic, eapol_key_packet->key_mic, 16);
    // Clear key MIC from EAPoL packet so hashcat can calulate MIC without preprocessing.
    // This is not documented in HCCAPX reference.
    // But it's based on 802.11i-2004 [8.5.2/h] and by analysing behaviour of cap2hccapx tool
    // MIC key on 77 bytes offset inside EAPoL-Key + 4 bytes EAPoL header.
    memset(&hccapx.eapol[81], 0x0, 16);
    return 0;
}

/**
 * @brief Handles first message of WPA handshake - from AP to STA
 * 
 * This message is from AP. It always contains ANonce.
 * 
 * @param eapol_key_packet parsed EAPoL-Key packet
 */
static void ap_message_m1(eapol_key_packet_t *eapol_key_packet){
    ESP_LOGD(TAG, "From AP M1");
    message_ap = 1;
    memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
}

/**
 * @brief Handles third message of WPA handshake - from AP to STA
 * 
 * @param eapol_packet 
 * @param eapol_key_packet 
 */
static void ap_message_m3(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ESP_LOGD(TAG, "From AP M3");
    message_ap = 3;
    if(message_ap == 0){
        // No AP message was processed yet. ANonce has to be copied into HCCAPX buffer.
        memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
    }
    if(eapol_source == 2){
        // EAPoL packet was already saved from message #2. No need to resave it.
        hccapx.message_pair = 2;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 3;
    if(message_sta == 2){
        hccapx.message_pair = 3;
    }
}

/**
 * @brief Handles messages from AP - handshake M1 and M3.
 * 
 * @param frame 
 * @param eapol_packet 
 * @param eapol_key_packet 
 */
static void ap_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if((!is_array_zero(hccapx.mac_sta, 6)) && (memcmp(frame->mac_header.addr1, hccapx.mac_sta, 6) != 0)){
        ESP_LOGE(TAG, "Different STA");
        return;
    }
    if(message_ap == 0){
        memcpy(hccapx.mac_ap, frame->mac_header.addr2, 6);
    }
    // Determine which message this is by Key MIC
    // Key MIC is always empty in M1 and always present in M3
    // Ref: 802.11i-2004 [8.5.3]
    if(is_array_zero(eapol_key_packet->key_mic, 16)){
        ap_message_m1(eapol_key_packet);
    } 
    else {
        ap_message_m3(eapol_packet, eapol_key_packet);
    }
}

/**
 * @brief Handles second message of handshake - from STA to AP.
 * 
 * Saves EAPoL packet as this is the first time key MIC is present.
 * Saves SNonce.
 * 
 * @param eapol_packet 
 * @param eapol_key_packet 
 */
static void sta_message_m2(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ESP_LOGD(TAG, "From STA M2");
    message_sta = 2;
    memcpy(hccapx.nonce_sta, eapol_key_packet->key_nonce, 32);
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 2;
    if(message_ap == 1){
        hccapx.message_pair = 0;
        return;
    }
}

/**
 * @brief Handles fourth message of the handshake. From STA to AP.
 * 
 * 
 * @param eapol_packet 
 * @param eapol_key_packet 
 */
static void sta_message_m4(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ESP_LOGD(TAG, "From STA M4");
    if((message_sta == 2) && (eapol_source != 0)){
        // If message 2 was already fully processed, there is no need to process M4 again 
        ESP_LOGD(TAG, "Already have M2, not worth");
        return;
    }
    if(message_ap == 0){
        // If there was no AP message processed yet, ANonce will be always missing.
        ESP_LOGE(TAG, "Not enought handshake messages received.");
        return;
    }
    if(eapol_source == 3){
        hccapx.message_pair = 4;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 4;
    if(message_ap == 1){
        hccapx.message_pair = 1;
    }
    if(message_ap == 3){
        hccapx.message_pair = 5;
    }
}

/**
 * @brief Handles messages from STA - M2 and M4
 * 
 * @param frame 
 * @param eapol_packet 
 * @param eapol_key_packet 
 */
static void sta_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if(is_array_zero(hccapx.mac_sta, 6)){
        memcpy(hccapx.mac_sta, frame->mac_header.addr2, 6);
    }
    else if(memcmp(frame->mac_header.addr2, hccapx.mac_sta, 6) != 0){
        ESP_LOGE(TAG, "Different STA");
        return;
    }
    // Determine which message this is by SNonce
    // SNonce is present in M2, empty in M4
    // Ref: 802.11i-2004 [8.5.3]
    if(!is_array_zero(eapol_key_packet->key_nonce, 16)){
        sta_message_m2(eapol_packet, eapol_key_packet);
    } 
    else {
        sta_message_m4(eapol_packet, eapol_key_packet);
    }
}

/**
 * @detail This component is a state machine, so this function can be used without knowing current state from outside.
 * WPA handshake pseudo-diagram:
 * @code{.unparsed}
 * AP           STA
 * M1 ---------> |
 * | <--------- M2
 * M3 ---------> |
 * | <--------- M4
 * @endcode
 * 
 * @param frame 
 */
void hccapx_serializer_add_frame(data_frame_t *frame){
    eapol_packet_t *eapol_packet = parse_eapol_packet(frame);
    eapol_key_packet_t *eapol_key_packet = parse_eapol_key_packet(eapol_packet);
    // Determine direction of the frame by comparing BSSID (addr3) with source address (addr2)
    if(memcmp(frame->mac_header.addr2, frame->mac_header.addr3, 6) == 0){
        ap_message(frame, eapol_packet, eapol_key_packet);
    } 
    else if(memcmp(frame->mac_header.addr1, frame->mac_header.addr3, 6) == 0){
        sta_message(frame, eapol_packet, eapol_key_packet);
    } 
    else {
        ESP_LOGE(TAG, "Unknown frame format. BSSID is not source nor destionation.");
    }
}