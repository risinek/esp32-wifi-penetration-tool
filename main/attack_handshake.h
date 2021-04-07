/**
 * @file attack_handshake.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control attacks on WPA handshake
 */
#ifndef ATTACK_HANDSHAKE_H
#define ATTACK_HANDSHAKE_H

#include "attack.h"

/**
 * @brief Available methods that can be chosen for the attack.
 * 
 */
typedef enum{
    ATTACK_HANDSHAKE_METHOD_ROGUE_AP,   ///< Method using rogue/duplicated AP utilising native ESP-IDF behaviour only
    ATTACK_HANDSHAKE_METHOD_BROADCAST,  ///< Method that takes advantage of WSL Bypasser component that bypass blocking mechanism in Wi-Fi Stack Libraries 
                                        /// to send raw 802.11 frames
    ATTACK_HANDSHAKE_METHOD_PASSIVE,    ///< Passive method that does not intervene communication on network, just passively capture handshake frames
} attack_handshake_methods_t;

/**
 * @brief Starts handshake attack with given attack config.
 * 
 * To stop handshake attack, call attack_handshake_stop().
 * 
 * @param attack_config attack config with valid ap_record and attack method chosen 
 */
void attack_handshake_start(attack_config_t *attack_config);

/**
 * @brief Stops handshake attack.
 * 
 * This function stops everything that attack_handshake_start() started and resets all values to default state.
 */
void attack_handshake_stop();

#endif