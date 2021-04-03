/**
 * @file attack_pmkid.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control PMKID attack.
 */
#ifndef ATTACK_PMKID_H
#define ATTACK_PMKID_H

#include "attack.h"

/**
 * @brief Starts PMKID attack with given attack_config_t.
 * 
 * To stop PMKID attack, call attack_pmkid_stop().
 * 
 * @param attack_config attack configuration with valid ap_record
 */
void attack_pmkid_start(attack_config_t *attack_config);
/**
 * @brief Stops PMKID attack.
 * 
 * It stops everything that attack_pmkid_start() started and resets values to original state.
 */
void attack_pmkid_stop();

#endif