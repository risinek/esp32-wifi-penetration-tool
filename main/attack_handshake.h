#ifndef ATTACK_HANDSHAKE_H
#define ATTACK_HANDSHAKE_H

#include "attack.h"

typedef enum{
    ATTACK_HANDSHAKE_METHOD_ROGUE_AP,
    ATTACK_HANDSHAKE_METHOD_BROADCAST,
} attack_handshake_methods_t;

void attack_handshake_start(attack_config_t *attack_config);
void attack_handshake_stop();

#endif