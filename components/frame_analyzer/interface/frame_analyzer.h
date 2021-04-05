/**
 * @file frame_analyzer.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface for frame analysis
 */
#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(FRAME_ANALYZER_EVENTS);

enum {
    DATA_FRAME_EVENT_EAPOLKEY_FRAME,
    DATA_FRAME_EVENT_PMKID
};

/**
 * @brief Search types for frame analyzer.
 * 
 * This hints frame analyzer what kind of information is requested.
 */
typedef enum {
    SEARCH_HANDSHAKE,
    SEARCH_PMKID
} search_type_t;

/**
 * @brief Starts frame analysis based on given search type and BSSID.
 * 
 * @param search_type type of information that are demanded
 * @param bssid target AP's BSSID
 */
void frame_analyzer_capture_start(search_type_t search_type, const uint8_t *bssid);

/**
 * @brief stops frame analysis
 * 
 */
void frame_analyzer_capture_stop();

#endif