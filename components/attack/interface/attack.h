#ifndef ATTACK_H
#define ATTACK_H

typedef enum {
    PASSIVE,
    HANDSHAKE,
    PMKID
} attack_type_t;

typedef enum {
    IDLE,
    RUNNING,
    FINISHED
} attack_status_t;

// typedef struct attack_config_t
    // ap_record_t *
    // attack_type_t
    // timeout

typedef struct {
    attack_status_t status;
} attack_result_t;

const attack_result_t *attack_get_result();
// void attack_set_result()
// void attack_run(attack_config_t *attack_config)

#endif