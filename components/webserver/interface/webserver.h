#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(WEBSERVER_EVENTS);
enum {
    WEBSERVER_EVENT_ATTACK_REQUEST,
    WEBSERVER_EVENT_ATTACK_RESET
};

typedef struct {
    uint8_t ap_record_id;
    uint8_t type;
    uint8_t timeout;
} attack_request_t;

void webserver_run();

#endif