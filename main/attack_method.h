/**
 * @file attack_method.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 *
 * @brief Provides interface for common methods used in various attacks
 */
#ifndef ATTACK_METHOD_H
#define ATTACK_METHOD_H

#include "MacContainer.h"
#include "attack.h"
#include "esp_wifi_types.h"

/**
 * @brief Starts periodic deauthentication frame broadcast
 *
 * @param ap_record array with target AP records which BSSID will be used in deauthentication frame
 * @param period_sec period of broadcast in seconds
 */
void attack_method_broadcast(const ap_records_t& ap_records, unsigned period_sec,
                             const MacContainer& staionMacsBlackList);

/**
 * @brief Stop periodic deauthentication frame broadcast
 */
void attack_method_broadcast_stop();

/**
 * @brief Starts duplicated AP with same BSSID as genuine AP from ap_record
 *
 * This will execute deauthentication attack for given AP.
 * @param ap_record array with target AP records that will be cloned/duplicated
 */
void attack_method_rogueap(const ap_records_t& ap_records, uint16_t per_ap_timeout);

/**
 * @brief Stop Rogue AP attack
 */
void attack_method_rogueap_stop();

#endif