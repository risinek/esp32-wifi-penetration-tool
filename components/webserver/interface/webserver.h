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

#include <functional>
#include <string>
#include <vector>

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(WEBSERVER_EVENTS);
enum { WEBSERVER_EVENT_ATTACK_REQUEST, WEBSERVER_EVENT_ATTACK_RESET };

/**
 * @brief Struct to deserialize attack request parameters
 *
 */
typedef struct {
  uint8_t type{0};
  uint8_t method{0};
  uint16_t timeout{0};
  uint16_t per_ap_timeout{0};
  uint8_t ap_records_len{0};
  uint8_t* ap_records_ids{nullptr};
} __attribute__((packed)) attack_request_t;

/**
 * @brief Initializes and starts webserver
 */
void webserver_run();

void setWebserverOtaRequestHandler(std::function<void(const std::string& param)> onOtaRequestHandler);
void setHTTPActivityHandler(std::function<void()> httpActivityHandler);

#endif  // WEBSERVER_H
