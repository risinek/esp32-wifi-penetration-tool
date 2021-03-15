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
    uint8_t type;
    uint8_t timeout;
    const wifi_ap_record_t *ap_record;
} attack_config_t;

typedef struct {
    uint8_t status;
    uint8_t type;
    uint8_t content_size;
    char *content;
} attack_result_t;

const attack_result_t *attack_get_result();
void attack_set_result(attack_status_t status);
void attack_init();
char *attack_alloc_result_content(unsigned size);

#endif