/**
 * @file webserver.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control and communicate with Webserver component
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_event.h"

#include <string>
#include <functional>

ESP_EVENT_DECLARE_BASE(WEBSERVER_EVENTS);
enum {
    WEBSERVER_EVENT_ATTACK_REQUEST,
    WEBSERVER_EVENT_ATTACK_RESET
};

/**
 * @brief Struct to deserialize attack request parameters 
 * 
 */
typedef struct {
      uint8_t type;
      uint8_t method;
      uint16_t timeout;
      uint16_t per_ap_timeout;
      uint8_t ap_records_len;
      uint8_t* ap_records_ids;
} __attribute__((packed)) attack_request_t;

/**
 * @brief Initializes and starts webserver 
 */
void webserver_run();

void setWebserverOnOtaRequestHandler(std::function<void(const std::string& param)> onOtaRequestHandler);

#endif  // WEBSERVER_H
