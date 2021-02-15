#include "attack.h"

static attack_result_t attack_result = { .status = IDLE };

const attack_result_t *attack_get_result() {
    return &attack_result;
}