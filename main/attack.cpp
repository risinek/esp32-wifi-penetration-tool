/**
 * @file attack.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-02
 * @copyright Copyright (c) 2021
 *
 * @brief Implements common attack wrapper.
 */

#include "attack.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "attack_dos.h"
#include "attack_handshake.h"
#include "attack_pmkid.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "webserver.h"
#include "wifi_controller.h"

namespace {
const char *TAG = "attack";
attack_status_t attack_status = {.state = READY, .type = (uint8_t)-1, .content_size = 0, .content = NULL};
esp_timer_handle_t attack_timeout_handle;
}  // namespace

bool gShouldLimitAttackLogs = false;

const attack_status_t *attack_get_status() { return &attack_status; }

void attack_update_status(attack_state_t state) {
  attack_status.state = state;
  if (state == FINISHED) {
    ESP_LOGD(TAG, "Stopping attack timeout timer");
    ESP_ERROR_CHECK(esp_timer_stop(attack_timeout_handle));
  }
}

void attack_append_status_content(uint8_t *buffer, unsigned size) {
  if (size == 0) {
    ESP_LOGE(TAG, "Size can't be 0 if you want to reallocate");
    return;
  }
  // temporarily save new location in case of realloc failure to preserve current content
  char *reallocated_content = (char *)realloc(attack_status.content, attack_status.content_size + size);
  if (reallocated_content == NULL) {
    ESP_LOGE(TAG, "Error reallocating status content! Status content may not be complete.");
    return;
  }
  // copy new data after current content
  memcpy(&reallocated_content[attack_status.content_size], buffer, size);
  attack_status.content = reallocated_content;
  attack_status.content_size += size;
}

char *attack_alloc_result_content(unsigned size) {
  attack_status.content_size = size;
  attack_status.content = (char *)malloc(size);
  return attack_status.content;
}

/**
 * @brief Callback function for attack timeout timer.
 *
 * This function is called when attack times out.
 * It updates attack status state to TIMEOUT.
 * It calls appropriate abort functions based on current attack type.
 * @param arg not used.
 */
static void attack_timeout(void *arg) {
  ESP_LOGD(TAG, "Attack timed out");

  attack_update_status(TIMEOUT);

  switch (attack_status.type) {
    case ATTACK_TYPE_PMKID:
      ESP_LOGI(TAG, "Aborting PMKID attack...");
      attack_pmkid_stop();
      break;
    case ATTACK_TYPE_HANDSHAKE:
      ESP_LOGI(TAG, "Abort HANDSHAKE attack...");
      attack_handshake_stop();
      break;
    case ATTACK_TYPE_PASSIVE:
      ESP_LOGI(TAG, "Abort PASSIVE attack...");
      break;
    case ATTACK_TYPE_DOS:
      ESP_LOGI(TAG, "Abort DOS attack...");
      attack_dos_stop();
      break;
    default:
      ESP_LOGE(TAG, "Unknown attack type. Not aborting anything");
  }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_REQUEST event.
 *
 * This function handles WEBSERVER_EVENT_ATTACK_REQUEST event from event loop.
 * It parses attack_request_t structure and set initial values to attack_status.
 * It sets attack state to RUNNING or RUNNING_INFINITELY.
 * It starts attack timeout timer.
 * It starts attack based on chosen type.
 *
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_REQUEST
 * @param event_data expects attack_request_t
 */
static void attack_request_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "Starting attack...");

  attack_request_t *attack_request = (attack_request_t *)event_data;
  ap_records_t ap_records;
  ap_records.len = attack_request->ap_records_len;
  ap_records.records = (const wifi_ap_record_t **)malloc(ap_records.len * sizeof(wifi_ap_record_t *));
  for (int i = 0; i < ap_records.len; ++i) {
    ap_records.records[i] = wifictl_get_ap_record(attack_request->ap_records_ids[i]);
    if (ap_records.records[i] == NULL) {
      ESP_LOGE(TAG, "NPE: No ap_record for id '%d'!", attack_request->ap_records_ids[i]);
      free(ap_records.records);
      free(attack_request->ap_records_ids);
      return;
    }
  }
  free(attack_request->ap_records_ids);

  attack_config_t attack_config = {.type = attack_request->type,
                                   .method = attack_request->method,
                                   .timeout = attack_request->timeout,
                                   .per_ap_timeout = attack_request->per_ap_timeout,
                                   .ap_records = ap_records};

  // TODO(alambin):
  // PRIORITIES:
  // 20, 23, 18, 3, 12, 21

  // TODO(alambin):
  // Bluetooth part:
  // V 1. Implement Bluetooth PIN/password/passcode request.
  //    Currently we can reset device by writing characteristic, which is not so convenient.
  //
  // WiFi part:
  // 2. Update WebUI to support all new freatures:
  //    X 1. ATTACK_DOS_METHOD_BROADCAST_MULTI_AP
  //    X 2. ATTACK_TYPE_STOP_ATTACK
  //    3. If attack is DOS, then we can run it forever by setting timeout to 0.
  //       Should display info that RougeAP attack can be stopped only by reset (hard or soft by connecting to ESP-32
  //       via Bluetooth)
  //    4. On WebUI side instead of setting ap_record_id as 1st field, it should be represented as list of size 1
  // 3. Remember clients who are connecting to WiFi (is it possible only when we are in RougeAP mode and only for
  //    those stations, which connect to this RougeAP?) and when sending deauf, send deauth to them explicitely (not
  //    via broadcast). Such a targetted frames will not be ignored by some devices. Not to blow up this list, we can
  //    clean it, say, once per day IF itexceeds specified size.
  //    I REALLY need MAC addresses from 5G network to add them to black list on UI later
  //    1. Make new end-point to read this list
  //    2. Example: D:\Temp\Arduino\esp-idf\examples\wifi\getting_started\softAP\main\softap_example_main.c
  //       Looks like we need to handle event WIFI_EVENT_AP_STACONNECTED in wifi_event_handler(), which is set by
  //       esp_event_handler_register
  // 4. Need to make sure that all my changes are not breaking existing code. Ex. that proper status of attack will
  //    be returned by attack_get_status(), that each attack really has proper status (remember, that now we have
  //    infinite attacks and ability to interrupt attacks)
  // 5. Make sure that timeout in WebUI is handled well (even in case of infinite attack).
  // V 6. Web logger. Refer to one of my previous project, where buffered logger was used. Create new end-point page
  //    (/log) and in response make simple page with text window containing buffered logs.
  //    Buffer should contain only the latest N lines of logs. Make size configurable (via menuconfig?).
  //    Try to find way to get stream of logs and send them not to Serial Port, but to this logger. May be some
  //    hook/callback is provided by ESP to handle all outgoing logs from system (esp_log_set_vprintf ???)?
  // V 7. Use config-time constants to set device ID (0-...) and use it to create constants for WiFi AP name, Bluetooth
  //    device name, IP address. So that we can easily generate binaries for multiple ESP32 devices
  //    How to change IP adress in index.html?
  // V 8. Extend WebUI so that for infinit attacks it will
  //    V 1. not show timeout window
  //    V 2. Shows status about infinit attack somewhere
  //    V 3. Show button to stop attack (refer to "resetAttack()", but without changes in UI)
  //    V 4. not let send any command except of STOP
  //    V 5. To properly handle refresh page, ESP32 should know about infinit status and should return proper new status
  //         code to WebUI on request of "/status"
  // V 9. Implement multiple RougeAP attack
  // v 10. Test infinit and regular multiple AP attacks
  // V 11. Test multiple AP attack where our router
  //     V 1. comes 1st in the list
  //     V 2. comes 2nd in the list
  // 12. "Black list of MACs" feature. Add MAC addresses list in WebUI. For those addresses need to send personal
  //     deauth frame for every AP during broadcast attack
  //     1. Also need to have black list of router's MACs. If one of them is detected, its SSID should be printed on
  //        WebUI. So, if user will change SSID, they will be anyway dosplayed.
  //        Ex. use pre-defined list of such MACs in WebUI. After scanning is done, WebUI can check list of returned
  //        SSIDs to find those ones from the black list
  //     2. It would be cool to send black list of router MACs to ESP32 so that it could scan from time to time, if
  //        network with given MAC appeared, then it is automatically added to the current attack.
  //        Or we can just run task from time to time, which checks original list of APs, read their MACs and makes
  //        sure they have the same names. If name is changed, then we should update appropriate entire
  //        wifi_ap_record_t record in origial list.
  // 13. Stability test (aster most of changes are done). Keep ESP32 running as long as possible, running different
  //     attacks. The goal is to make sure that after different use cases it is still up and running
  // 14. "Stop attack" button in WebUIdoesn't make sense in method which includes Rogue AP, because we will simple can
  //     not send any request to ESP32. Actually after initiating such attack the only thing we can do in UI is to show
  //     message that ESP32's WiFi will be off during all attack. The only way to make it available again - reboot via
  //     Bluetooth
  //     1. Need to analyze other cases, when ESP32 will not be available and probably adapt behavior of UI for it
  // V 15. Check if there is such thing as Bluetooth logger for ESP32
  //     This one looks like without buffering?
  //     https://github.com/espressif/arduino-esp32/blob/master/libraries/BluetoothSerial/examples/SerialToSerialBT/SerialToSerialBT.ino
  //     V 1. Too many dependencies on Arduino. Try to use ESP SPP (serial port) example
  //     V 2. BT trasfering (not only receiving). Refer to bt_spp_initiator code
  //     V 3. Implement buferisation (refer to another my code for ESP32)
  //     V 4. Implement rest logging logic: get logs from ESP32 system, buffer them and periodiaclly send to BT
  //     V 5. Implement parsing of commands received from BT. Don't forget reimplement existing 'reset'
  //     V 6. Implement output of all supported commands when BT terminal is connected
  // 16. Blink red LED few times at startup (FREERTOS task?) and then turn it off
  // 17. BUG: if DOS Braadcast attack is in progress and you refresh page, list of APs looks like read by ESP (see its
  //     logs), but not displayed on WebUI. Connection to ESP is lost. May be ESP kills its AP when tries to send this
  //     list to WebUI? Ah, may be incorrect handling of request to "/status" when we start WebUI during attack?
  //     Hm. Not wlways reproducible
  // 18. Bluetooth new command - "stop" to stop attack and restore original AP (it is most probably done as part of
  //     attack_timeout()/stop_attack()). Can we just call attack_reset_handler()?
  //     Q: what;s difference from regular reboot?
  // 19. New commands for Bluetooth - blink LED, start LED, stop LED. To make device easier to be found (if forgot
  //     where is it)
  // 20. OTA
  // 21. Return attack status in JSON
  //     1. Adapt WebUI
  //     2. New BT command "getattackstatus"
  // 22. Update Read.me file. Add info about new features, including
  //     - It is possible to build firmware for mutiple ESP32 devices, which will run the same software. These firmares
  //       should differ by DEVICE_ID, to make ESP32 use different WiFi access points names, Bluetooth device
  //       names, IP addresses, etc. DEVICE_ID can be set in sdkconfig or provided in common like
  //       (ex. "idf.py build -DDEVICE_ID=2")
  // 23. How to improve WiFi antenna?
  //     Can try this:
  //     1. https://peterneufeld.wordpress.com/2021/10/14/esp32-range-extender-antenna-modification/
  //        It gives just 3 db (+1.5 dB with 4 mm shorter wire - refer to comments):
  //     2. https://community.home-assistant.io/t/how-to-add-an-external-antenna-to-an-esp-board/131601
  //        Looks more promissing, up to -90 -> -65 !!!. But need solder iron and clue.
  //        Possible(?) antenna ("esp32 ăng ten") - https://shopee.vn/%C4%82ng-Ten-Khu%E1%BA%BFch-%C4%90%E1%BA%A1i-T%C3%ADn-Hi%E1%BB%87u-FM-AM-G%E1%BA%AFn-N%C3%B3c-Xe-H%C6%A1i-Benz-Bmw-Audi-Toyota-9-11-16-Inch-i.267737919.4436223296?sp_atk=4bac905a-a588-4f02-9af3-0359c20b77a3&xptdk=4bac905a-a588-4f02-9af3-0359c20b77a3
  //     4. As an option - buy ESP-WHROOM-32U. But I don't want to spend more money.
  // 24. Make pre-configured attack per DEVICE_ID. Ex. after reboot if device is not configured for 5-10 min, it starts
  //     pre-configured attack. HTTP or BT interactions reset counter

  // 5. Broadcast attack can be extended on multiple Access Points.
  //    We can introduce new method - ATTACK_DOS_METHOD_BROADCAST. If it is selected in WebUI, we can allow to
  //    select multiple APs. Then we send more data in HTTP Post request: ap_list_len, ap0, ap1, ...
  //    Sending is not a problem, but how to read data with various sise on ESp32 side?
  //    Now in uri_run_attack_post_handler() we are using httpd_req_recv, which reads only specified
  //    (sizeof(attack_request_t)) amount of bytes. In case we can call this method 2 times, then in 1st call we
  //    figure out size of list of APs and in 2nd call we can read this list (Answer - can NOT).
  //    Need either find approve in documentation, either try it with board.
  //    Idea: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
  //         "Use content_len provided in httpd_req_t structure to know the length of data to be fetched"
  //         So, attack_request_t should be
  // typedef struct {
  //     uint8_t type;
  //     uint8_t method;
  //     uint8_t timeout;
  //     uint8_t ap_record_len;
  //     uint8_t* ap_record_ids;
  // } attack_request_t;
  //
  // And code to read data should be
  // attack_request_t attack_request;
  // void *rawData = malloc(req->content_len);
  // attack_request_t* raw_attack_request = rawData;
  // httpd_req_recv(req, (char *)rawData, req->content_len);

  // attack_request.type = raw_attack_request.type;
  // attack_request.method = raw_attack_request.method;
  // attack_request.tytimeoute = raw_attack_request.timeout;
  // attack_request.ap_record_len = raw_attack_request.ap_record_len;
  // attack_request.ap_record_ids = malloc(attack_request.ap_record_len);

  // uint8_t* currentId = &(uint8_t)(raw_attack_request.ap_record_ids)
  // for (int i = 0; i < attack_request.ap_record_len; ++i) {
  //     attack_request.ap_record_ids[i] = *currentId;
  //     currentId++;
  // }
  //
  // Try this code FIRST to pass 1 value and print logs. If it works, extend to multiple values and make sure
  // by logs that it works

  // DO NOT FORGET TO FREE attack_request.ap_record_ids SOMEWHERE

  // Set timeout to stop attack.
  // In case it is DOS broadcast and timeout is 0, do not set timer and let attack to run forever
  // NOTE! In case of RougeAP attack, new AP will be established forever. The only way to stop it is reset: either
  // hard (button on board), either soft. Currently to trigger soft reset you need to connect to ESP-32 by Bluetooth
  if (((attack_config.timeout == 0) && (attack_config.type == ATTACK_TYPE_DOS))) {
    ESP_LOGI(TAG, "Timeout is set to 0. Atack will not finish until reboot (via Bluetooth) or 'reset' command");
  } else {
    // Before starting timer, make sure previous attack is finished. If not - finish it by simulating timeout
    esp_err_t start_result = esp_timer_start_once(attack_timeout_handle, attack_config.timeout * 1000000);
    if (start_result != ESP_OK) {
      if (start_result == ESP_ERR_INVALID_STATE) {
        // Previous attack is still in progress. Try to stop it and start timer again
        esp_timer_stop(attack_timeout_handle);
        attack_timeout(NULL);  // Simulate timeout event

        ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, attack_config.timeout * 1000000));
      } else {
        ESP_ERROR_CHECK(start_result);
        return;
      }
    }
  }

  attack_status.state =
      (((attack_config.timeout == 0) && (attack_config.type == ATTACK_TYPE_DOS))) ? RUNNING_INFINITELY : RUNNING;
  attack_status.type = attack_config.type;

  // start attack based on it's type
  switch (attack_config.type) {
    case ATTACK_TYPE_PMKID:
      attack_pmkid_start(&attack_config);
      break;
    case ATTACK_TYPE_HANDSHAKE:
      attack_handshake_start(&attack_config);
      break;
    case ATTACK_TYPE_PASSIVE:
      ESP_LOGW(TAG, "ATTACK_TYPE_PASSIVE not implemented yet!");
      break;
    case ATTACK_TYPE_DOS:
      attack_dos_start(&attack_config);
      break;
    default:
      ESP_LOGE(TAG, "Unknown attack type!");
      free(attack_config.ap_records.records);
      return;
  }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_RESET event.
 *
 * This callback resets attack status by freeing previously allocated status content and putting attack to READY state.
 *
 * @param args not used
 * @param event_base expects WEBSERVER_EVENTS
 * @param event_id expects WEBSERVER_EVENT_ATTACK_RESET
 * @param event_data not used
 */
static void attack_reset_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Resetting attack...");

  // Stop timer if it was initiated by previous attack.
  esp_timer_stop(attack_timeout_handle);
  // Simulate timeout event. We should not call attack_update_status(FINISHED), because it will not finalize
  // attack properly (ex. it will not call attack_dos_stop(), etc.)
  attack_timeout(NULL);

  if (attack_status.content) {
    free(attack_status.content);
    attack_status.content = NULL;
  }
  attack_status.content_size = 0;
  attack_status.type = -1;
  attack_status.state = READY;
}

/**
 * @brief Initialises common attack resources.
 *
 * Creates attack timeout timer.
 * Registers event loop event handlers.
 */
void attack_init() {
  const esp_timer_create_args_t attack_timeout_args = {.callback = &attack_timeout};
  ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

  ESP_ERROR_CHECK(
      esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
  ESP_ERROR_CHECK(
      esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}

// TODO(alambn): can we do it in better way than extern var amound multiple components?
void attack_limit_logs(bool isLimited) { gShouldLimitAttackLogs = isLimited; }