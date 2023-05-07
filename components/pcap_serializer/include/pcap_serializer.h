/**
 * @file pcap_serializer.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to generate PCAP formatted binary from raw frame bytes 
 */
#ifndef PCAP_SERIALIZER_H
#define PCAP_SERIALIZER_H

#include <stdint.h>

/**
 * @brief PCAP global header
 * 
 * @see Ref: https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat#global-header
 */
typedef struct {
            uint32_t magic_number;   /* magic number */
            uint16_t version_major;  /* major version number */
            uint16_t version_minor;  /* minor version number */
            int32_t  thiszone;       /* GMT to local correction */
            uint32_t sigfigs;        /* accuracy of timestamps */
            uint32_t snaplen;        /* max length of captured packets, in octets */
            uint32_t network;        /* data link type */
} pcap_global_header_t;

/**
 * @brief PCAP record header
 * 
 * @see Ref: https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat
 */
typedef struct {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcap_record_header_t;

/**
 * @brief Prepares new empty buffer for PCAP formatted binary data. 
 * 
 * Has always to be called before pcap_serializer_append_frame()
 * @return uint8_t* pointer to newly allocated PCAP buffer.
 * @return \c NULL initialisation failed
 */
uint8_t *pcap_serializer_init();

/**
 * @brief Appends new frame to existing PCAP buffer.
 * 
 * Expects pcap_serializer_append_frame() was already called.
 * @param buffer frame buffer that should be appended to PCAP
 * @param size size of frame buffer
 * @param ts_usec timestamp of captured frame in microseconds
 */
void pcap_serializer_append_frame(const uint8_t *buffer, unsigned size, unsigned ts_usec);

/**
 * @brief Frees PCAP buffer and resets all values.
 * 
 * After calling this function, you have to call pcap_serializer_init() to append new frames again.
 * 
 */
void pcap_serializer_deinit();

/**
 * @brief Returns size of PCAP buffer in bytes
 * 
 * @return unsigned
 */
unsigned pcap_serializer_get_size();

/**
 * @brief Return pointer to PCAP buffer
 * 
 * @return uint8_t* 
 */
uint8_t *pcap_serializer_get_buffer();

#endif