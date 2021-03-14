#ifndef ATTACK_H
#define ATTACK_H

#include "esp_wifi_types.h"

typedef enum {
    ATTACK_TYPE_PASSIVE,
    ATTACK_TYPE_HANDSHAKE,
    ATTACK_TYPE_PMKID
} attack_type_t;

typedef enum {
    READY,
    RUNNING,
    FINISHED,
    TIMEOUT
} attack_status_t;

typedef struct {
    attack_type_t type;
    int timeout;
    const wifi_ap_record_t *ap_record;
} attack_config_t;

typedef struct {
    attack_status_t status;
    attack_type_t type;
    size_t content_size;
    char *content;
} attack_result_t;

const attack_result_t *attack_get_result();
void attack_set_result(attack_status_t status);
void attack_run(const attack_config_t attack_config);

#endif